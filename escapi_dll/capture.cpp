#include <initguid.h>
#include <cguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <locale>
#include <codecvt>

#include <shlwapi.h> // QITAB and friends
#include <objbase.h> // IID_PPV_ARGS and friends
#include <dshow.h>   // IAMVideoProcAmp and friends

#include <math.h>

#define ESCAPI_DEFINITIONS_ONLY
#include "escapi.h"

#include "choosedeviceparam.h"
#include "conversion.h"
#include "capture.h"
#include "scopedrelease.h"
#include "videobufferlock.h"

#define DO_OR_DIE                  \
	{                              \
		if (mErrorLine)            \
			return hr;             \
		if (!SUCCEEDED(hr))        \
		{                          \
			mErrorLine = __LINE__; \
			mErrorCode = hr;       \
			return hr;             \
		}                          \
	}
#define DO_OR_DIE_CRITSECTION                \
	{                                        \
		if (mErrorLine)                      \
		{                                    \
			LeaveCriticalSection(&mCritsec); \
			return hr;                       \
		}                                    \
		if (!SUCCEEDED(hr))                  \
		{                                    \
			LeaveCriticalSection(&mCritsec); \
			mErrorLine = __LINE__;           \
			mErrorCode = hr;                 \
			return hr;                       \
		}                                    \
	}

CaptureClass::CaptureClass(IMFActivate* device)
{
	mRefCount = 1;
	InitializeCriticalSection(&mCritsec);
	mReader = nullptr;
	mSource = nullptr;
	mDefaultStride = 0;
	mConvertFn = nullptr;
	mCaptureBuffer = nullptr;
	mCaptureBufferWidth = 0;
	mCaptureBufferHeight = 0;
	mErrorLine = 0;
	mErrorCode = 0;
	mBadIndices = 0;
	mMaxBadIndices = 16;
	mBadIndex.reserve(mMaxBadIndices);
	mUsedIndex = 0;
	mRedoFromStart = 0;
	gDoCapture = 0;
	gOptions = 0;

	setup();
	wcname = updatename(device);
}

CaptureClass::~CaptureClass()
{
	deinitCapture();
	DeleteCriticalSection(&mCritsec);
}

// IUnknown methods
STDMETHODIMP CaptureClass::QueryInterface(REFIID aRiid, void** aPpv)
{
	static const QITAB qit[] = {
		QITABENT(CaptureClass, IMFSourceReaderCallback),
		{},
	};
	return QISearch(this, qit, aRiid, aPpv);
}

STDMETHODIMP_(ULONG) CaptureClass::AddRef()
{
	return InterlockedIncrement(&mRefCount);
}

STDMETHODIMP_(ULONG) CaptureClass::Release()
{
	ULONG count = InterlockedDecrement(&mRefCount);
	if (count == 0)
	{
		delete this;
	}
	// For thread safety, return a temporary variable.
	return count;
}

// IMFSourceReaderCallback methods
STDMETHODIMP CaptureClass::OnReadSample(HRESULT aStatus, DWORD aStreamIndex, DWORD aStreamFlags,
                                        LONGLONG aTimestamp, IMFSample* aSample)
{
	HRESULT hr = S_OK;
	IMFMediaBuffer* mediabuffer = nullptr;

	if (FAILED(aStatus))
	{
		// Bug workaround: some resolutions may just return error.
		// http://stackoverflow.com/questions/22788043/imfsourcereader-giving-error-0x80070491-for-some-resolutions
		// we fix by marking the resolution bad and retrying, which should use the next best match.
		mRedoFromStart = 1;
		if (mBadIndices == mMaxBadIndices)
		{
			mMaxBadIndices *= 2;
			mBadIndex.reserve(mMaxBadIndices);
		}
		mBadIndex[mBadIndices] = mUsedIndex;
		mBadIndices++;
		return aStatus;
	}

	EnterCriticalSection(&mCritsec);

	if (SUCCEEDED(aStatus))
	{
		if (gDoCapture == -1)
		{
			if (aSample)
			{
				// Get the video frame buffer from the sample.

				hr = aSample->GetBufferByIndex(0, &mediabuffer);
				ScopedRelease<IMFMediaBuffer> mediabuffer_s(mediabuffer);

				DO_OR_DIE_CRITSECTION;

				// Draw the frame.

				if (mConvertFn)
				{
					VideoBufferLock buffer(mediabuffer); // Helper object to lock the video buffer.

					BYTE* scanline0 = nullptr;
					LONG stride = 0;
					hr = buffer.LockBuffer(mDefaultStride, mCaptureBufferHeight, &scanline0,
					                       &stride);

					DO_OR_DIE_CRITSECTION;

					mConvertFn((BYTE*)mCaptureBuffer, mCaptureBufferWidth * 4, scanline0, stride,
					           mCaptureBufferWidth, mCaptureBufferHeight);
				}
				else
				{
					// No convert function?
					if (gOptions & CAPTURE_OPTION_RAWDATA)
					{
						// Ah ok, raw data was requested, so let's copy it then.

						VideoBufferLock buffer(
						    mediabuffer); // Helper object to lock the video buffer.
						BYTE* scanline0 = nullptr;
						LONG stride = 0;
						hr = buffer.LockBuffer(mDefaultStride, mCaptureBufferHeight, &scanline0,
						                       &stride);
						if (stride < 0)
						{
							scanline0 += stride * mCaptureBufferHeight;
							stride = -stride;
						}
						LONG bytes = stride * mCaptureBufferHeight;
						CopyMemory(mCaptureBuffer, scanline0, bytes);
					}
				}

				int i, j;
				int* dst = (int*)gParams.mTargetBuf;
				int* src = (int*)mCaptureBuffer;
				for (i = 0; i < gParams.mHeight; i++)
				{
					for (j = 0; j < gParams.mWidth; j++, dst++)
					{
						*dst =
						    src[(i * mCaptureBufferHeight / gParams.mHeight) * mCaptureBufferWidth +
						        (j * mCaptureBufferWidth / gParams.mWidth)];
					}
				}
				gDoCapture = 1;
			}
		}
	}

	// Request the next frame.
	hr = mReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
	                         nullptr, // actual
	                         nullptr, // flags
	                         nullptr, // timestamp
	                         nullptr  // sample
	);

	DO_OR_DIE_CRITSECTION;

	LeaveCriticalSection(&mCritsec);

	return hr;
}

STDMETHODIMP CaptureClass::OnEvent(DWORD, IMFMediaEvent*)
{
	return S_OK;
}

STDMETHODIMP CaptureClass::OnFlush(DWORD)
{
	return S_OK;
}

int CaptureClass::escapiPropToMFProp(int aProperty)
{
	int prop = 0;
	switch (aProperty)
	{
			// case CAPTURE_BRIGHTNESS:
		default:
			prop = VideoProcAmp_Brightness;
			break;
		case CAPTURE_CONTRAST:
			prop = VideoProcAmp_Contrast;
			break;
		case CAPTURE_HUE:
			prop = VideoProcAmp_Hue;
			break;
		case CAPTURE_SATURATION:
			prop = VideoProcAmp_Saturation;
			break;
		case CAPTURE_SHARPNESS:
			prop = VideoProcAmp_Sharpness;
			break;
		case CAPTURE_GAMMA:
			prop = VideoProcAmp_Gamma;
			break;
		case CAPTURE_COLORENABLE:
			prop = VideoProcAmp_ColorEnable;
			break;
		case CAPTURE_WHITEBALANCE:
			prop = VideoProcAmp_WhiteBalance;
			break;
		case CAPTURE_BACKLIGHTCOMPENSATION:
			prop = VideoProcAmp_BacklightCompensation;
			break;
		case CAPTURE_GAIN:
			prop = VideoProcAmp_Gain;
			break;
		case CAPTURE_PAN:
			prop = CameraControl_Pan;
			break;
		case CAPTURE_TILT:
			prop = CameraControl_Tilt;
			break;
		case CAPTURE_ROLL:
			prop = CameraControl_Roll;
			break;
		case CAPTURE_ZOOM:
			prop = CameraControl_Zoom;
			break;
		case CAPTURE_EXPOSURE:
			prop = CameraControl_Exposure;
			break;
		case CAPTURE_IRIS:
			prop = CameraControl_Iris;
			break;
		case CAPTURE_FOCUS:
			prop = CameraControl_Focus;
			break;
	}
	return prop;
}

int CaptureClass::setProperty(int aProperty, float aValue, int aAuto)
{
	HRESULT hr;
	IAMVideoProcAmp* procAmp = nullptr;
	IAMCameraControl* control = nullptr;

	int prop = escapiPropToMFProp(aProperty);

	if (aProperty < CAPTURE_PAN)
	{
		hr = mSource->QueryInterface(IID_PPV_ARGS(&procAmp));
		if (SUCCEEDED(hr))
		{
			long min, max, step, def, caps;
			hr = procAmp->GetRange(prop, &min, &max, &step, &def, &caps);

			if (SUCCEEDED(hr))
			{
				LONG val = (long)floor(min + (max - min) * aValue);
				if (aAuto)
					val = def;
				hr = procAmp->Set(prop, val,
				                  aAuto ? VideoProcAmp_Flags_Auto : VideoProcAmp_Flags_Manual);
			}
			procAmp->Release();
			return !!SUCCEEDED(hr);
		}
	}
	else
	{
		hr = mSource->QueryInterface(IID_PPV_ARGS(&control));
		if (SUCCEEDED(hr))
		{
			long min, max, step, def, caps;
			hr = control->GetRange(prop, &min, &max, &step, &def, &caps);

			if (SUCCEEDED(hr))
			{
				LONG val = (long)floor(min + (max - min) * aValue);
				if (aAuto)
					val = def;
				hr = control->Set(prop, val,
				                  aAuto ? VideoProcAmp_Flags_Auto : VideoProcAmp_Flags_Manual);
			}
			control->Release();
			return !!SUCCEEDED(hr);
		}
	}

	return 1;
}

int CaptureClass::getProperty(int aProperty, float& aValue, int& aAuto)
{
	HRESULT hr;
	IAMVideoProcAmp* procAmp = nullptr;
	IAMCameraControl* control = nullptr;

	aAuto = 0;
	aValue = -1;

	int prop = escapiPropToMFProp(aProperty);

	if (aProperty < CAPTURE_PAN)
	{
		hr = mSource->QueryInterface(IID_PPV_ARGS(&procAmp));
		if (SUCCEEDED(hr))
		{
			long min, max, step, def, caps;
			hr = procAmp->GetRange(prop, &min, &max, &step, &def, &caps);

			if (SUCCEEDED(hr))
			{
				long v = 0, f = 0;
				hr = procAmp->Get(prop, &v, &f);
				if (SUCCEEDED(hr))
				{
					aValue = (v - min) / (float)(max - min);
					aAuto = !!(f & VideoProcAmp_Flags_Auto);
				}
			}
			procAmp->Release();
			return 0;
		}
	}
	else
	{
		hr = mSource->QueryInterface(IID_PPV_ARGS(&control));
		if (SUCCEEDED(hr))
		{
			long min, max, step, def, caps;
			hr = control->GetRange(prop, &min, &max, &step, &def, &caps);

			if (SUCCEEDED(hr))
			{
				long v = 0, f = 0;
				hr = control->Get(prop, &v, &f);
				if (SUCCEEDED(hr))
				{
					aValue = (v - min) / (float)(max - min);
					aAuto = !!(f & VideoProcAmp_Flags_Auto);
				}
			}
			control->Release();
			return 0;
		}
	}

	return 1;
}

BOOL CaptureClass::isFormatSupported(REFGUID aSubtype) const
{
	DWORD i;
	for (i = 0; i < gConversionFormats; i++)
	{
		if (aSubtype == gFormatConversions[i].mSubtype)
		{
			return TRUE;
		}
	}
	return FALSE;
}

HRESULT CaptureClass::getFormat(DWORD aIndex, GUID* aSubtype) const
{
	if (aIndex < gConversionFormats)
	{
		*aSubtype = gFormatConversions[aIndex].mSubtype;
		return S_OK;
	}
	return MF_E_NO_MORE_TYPES;
}

HRESULT CaptureClass::setConversionFunction(REFGUID aSubtype)
{
	mConvertFn = nullptr;

	// If raw data is desired, skip conversion
	if (gOptions & CAPTURE_OPTION_RAWDATA)
		return S_OK;

	for (DWORD i = 0; i < gConversionFormats; i++)
	{
		if (gFormatConversions[i].mSubtype == aSubtype)
		{
			mConvertFn = gFormatConversions[i].mXForm;
			return S_OK;
		}
	}

	return MF_E_INVALIDMEDIATYPE;
}

HRESULT CaptureClass::setVideoType(IMFMediaType* aType)
{
	HRESULT hr = S_OK;
	GUID subtype = {};

	// Find the video subtype.
	hr = aType->GetGUID(MF_MT_SUBTYPE, &subtype);

	DO_OR_DIE;

	// Choose a conversion function.
	// (This also validates the format type.)

	hr = setConversionFunction(subtype);

	DO_OR_DIE;

	//
	// Get some video attributes.
	//

	subtype = GUID_NULL;

	UINT32 width = 0;
	UINT32 height = 0;

	// Get the subtype and the image size.
	hr = aType->GetGUID(MF_MT_SUBTYPE, &subtype);

	DO_OR_DIE;

	hr = MFGetAttributeSize(aType, MF_MT_FRAME_SIZE, &width, &height);

	DO_OR_DIE;

	hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &mDefaultStride);

	mCaptureBuffer = new unsigned int[width * height];
	mCaptureBufferWidth = width;
	mCaptureBufferHeight = height;

	DO_OR_DIE;

	return hr;
}

int CaptureClass::isMediaOk(IMFMediaType* aType, unsigned int aIndex)
{
	HRESULT hr = S_OK;

	unsigned int i;
	for (i = 0; i < mBadIndices; i++)
		if (mBadIndex.at(i) == aIndex)
			return FALSE;

	BOOL found = FALSE;
	GUID subtype = {};

	hr = aType->GetGUID(MF_MT_SUBTYPE, &subtype);

	DO_OR_DIE;

	// Do we support this type directly?
	if (isFormatSupported(subtype))
	{
		found = TRUE;
	}
	else
	{
		// Can we decode this media type to one of our supported
		// output formats?

		for (i = 0;; i++)
		{
			// Get the i'th format.
			hr = getFormat(i, &subtype);

			if (FAILED(hr))
			{
				break;
			}

			hr = aType->SetGUID(MF_MT_SUBTYPE, subtype);

			if (FAILED(hr))
			{
				break;
			}

			// Try to set this type on the source reader.
			hr = mReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr,
			                                  aType);

			if (SUCCEEDED(hr))
			{
				found = TRUE;
				break;
			}
		}
	}
	return found;
}

int CaptureClass::scanMediaTypes(unsigned int aWidth, unsigned int aHeight)
{
	HRESULT hr;
	HRESULT nativeTypeErrorCode = S_OK;
	DWORD count = 0;
	int besterror = 0xfffffff;
	int bestfit = 0;

	resolutions.clear();
	while (nativeTypeErrorCode == S_OK && besterror)
	{
		IMFMediaType* nativeType = nullptr;
		nativeTypeErrorCode = mReader->GetNativeMediaType(
		    (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, count, &nativeType);
		ScopedRelease<IMFMediaType> nativeType_s(nativeType);

		if (nativeTypeErrorCode != S_OK)
			continue;

		// get the media type
		GUID nativeGuid = {};
		hr = nativeType->GetGUID(MF_MT_SUBTYPE, &nativeGuid);

		if (FAILED(hr))
			return bestfit;

		if (isMediaOk(nativeType, count))
		{
			UINT32 width, height;
			hr = MFGetAttributeSize(nativeType, MF_MT_FRAME_SIZE, &width, &height);

			if (FAILED(hr))
				return bestfit;

			resolutions.push_back(resolution(width, height));
			int error = 0;

			// prefer (hugely) to get too much than too little data..

			if (aWidth < width)
				error += (width - aWidth);
			if (aHeight < height)
				error += (height - aHeight);
			if (aWidth > width)
				error += (aWidth - width) * 2;
			if (aHeight > height)
				error += (aHeight - height) * 2;

			if (aWidth == width && aHeight == height) // ..but perfect match is a perfect match
				error = 0;

			if (besterror > error)
			{
				besterror = error;
				bestfit = count;
			}
			/*
			char temp[1024];
			sprintf(temp, "%d x %d, %x:%x:%x:%x %d %d\n", width, height, nativeGuid.Data1,
			nativeGuid.Data2, nativeGuid.Data3, nativeGuid.Data4, bestfit == count, besterror);
			OutputDebugStringA(temp);
			*/
		}

		count++;
	}
	return bestfit;
}

std::vector<CaptureClass::resolution> CaptureClass::getSupportedResolutions()
{
	if (resolutions.empty())
		initCapture(&gParams, gOptions);

	return resolutions;
}

HRESULT CaptureClass::initCapture(const struct SimpleCapParams* aParams, unsigned int aOptions)
{
	HRESULT hr;

	deinitCapture();

	gParams = *aParams;
	gOptions = aOptions;

	// use param.ppDevices[0]
	IMFAttributes* attributes = nullptr;
	EnterCriticalSection(&mCritsec);

	ChooseDeviceParam param = {};
	IMFActivate* device = get(param);
	if (!device)
	{
		LeaveCriticalSection(&mCritsec);
		mErrorLine = __LINE__;
		mErrorCode = E_FAIL;
		return mErrorCode;
	}
	hr = device->ActivateObject(__uuidof(IMFMediaSource), (void**)&mSource);

	DO_OR_DIE_CRITSECTION;

	hr = MFCreateAttributes(&attributes, 3);
	ScopedRelease<IMFAttributes> attributes_s(attributes);

	DO_OR_DIE_CRITSECTION;

	hr = attributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);

	DO_OR_DIE_CRITSECTION;

	hr = attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);

	DO_OR_DIE_CRITSECTION;

	hr = MFCreateSourceReaderFromMediaSource(mSource, attributes, &mReader);

	DO_OR_DIE_CRITSECTION;

	LeaveCriticalSection(&mCritsec);
	changeResolution(gParams.mWidth, gParams.mHeight);

	return 0;
}

bool CaptureClass::changeResolution(size_t width, size_t height)
{
	HRESULT hr;
	IMFMediaType* type = nullptr;

	EnterCriticalSection(&mCritsec);

	gParams.mWidth = width;
	gParams.mHeight = height;
	size_t preferredmode = scanMediaTypes(gParams.mWidth, gParams.mHeight);
	mUsedIndex = preferredmode;

	hr = mReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, preferredmode,
	                                 &type);
	ScopedRelease<IMFMediaType> type_s(type);

	DO_OR_DIE_CRITSECTION;

	hr = setVideoType(type);

	DO_OR_DIE_CRITSECTION;

	hr = mReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, type);

	DO_OR_DIE_CRITSECTION;

	hr = mReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr,
	                         nullptr, nullptr);

	DO_OR_DIE_CRITSECTION;

	LeaveCriticalSection(&mCritsec);
}

void CaptureClass::deinitCapture()
{
	EnterCriticalSection(&mCritsec);

	if (mReader)
	{
		mReader->Release();
		mReader = nullptr;
	}

	if (mSource)
	{
		mSource->Shutdown();
		mSource->Release();
		mSource = nullptr;
	}
	delete[] mCaptureBuffer;
	mCaptureBuffer = nullptr;

	LeaveCriticalSection(&mCritsec);
}

std::wstring CaptureClass::name() const
{
	return wcname;
}

std::string CaptureClass::cname() const
{
	const std::wstring wname = name();
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;

	// use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.to_bytes(wname);
}

std::wstring CaptureClass::updatename(IMFActivate* device)
{
	std::wstring wcname;
	HRESULT hr;
	WCHAR* name = 0;
	UINT32 namelen = 255;

	hr = device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &namelen);
	if (SUCCEEDED(hr) && name)
	{
		for (size_t x = 0; x < namelen; x++)
			wcname += name[x];

		CoTaskMemFree(name);
	}
	return wcname;
}

bool CaptureClass::getall(ChooseDeviceParam& param)
{
	HRESULT hr;

	if (!setup())
	{
		return false;
	}
	// choose device
	IMFAttributes* attributes = nullptr;
	hr = MFCreateAttributes(&attributes, 1);
	ScopedRelease<IMFAttributes> attributes_s(attributes);

	if (FAILED(hr))
		return false;

	hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
	                         MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr))
		return false;

	hr = MFEnumDeviceSources(attributes, &param.mDevices, &param.mCount);

	if (FAILED(hr))
		return false;

	return true;
}

bool CaptureClass::setup()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr))
	{
		return false;
	}
	hr = MFStartup(MF_VERSION);
	return SUCCEEDED(hr);
}

IMFActivate* CaptureClass::get(ChooseDeviceParam& param)
{
	if (!getall(param))
	{
		return nullptr;
	}

	for (size_t x = 0; x < param.mCount; x++)
	{
		const std::wstring name = updatename(param.mDevices[x]);
		if (name.compare(wcname) == 0)
		{
			return param.mDevices[x];
		}
	}

	return nullptr;
}
