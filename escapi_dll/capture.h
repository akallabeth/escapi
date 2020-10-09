#pragma once

#include <initguid.h>
#include <cguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <vector>
#include <string>
#include <mutex>

#include <escapi.h>
#include "conversion.h"
#include "choosedeviceparam.h"

class CaptureClass : public IMFSourceReaderCallback
{
  public:
	struct resolution
	{
		size_t w;
		size_t h;

		resolution(size_t width, size_t height) : w(width), h(height)
		{
		}
	};

  public:
	static bool getall(ChooseDeviceParam& param);

  private:
	static bool setup();

  public:
	CaptureClass(IMFActivate* device);
	CaptureClass(const CaptureClass&) = delete;

	virtual ~CaptureClass();

	STDMETHODIMP QueryInterface(REFIID aRiid, void** aPpv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP OnReadSample(HRESULT aStatus, DWORD aStreamIndex, DWORD aStreamFlags,
	                          LONGLONG aTimestamp, IMFSample* aSample);
	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*);
	STDMETHODIMP OnFlush(DWORD);
	int escapiPropToMFProp(CAPTURE_PROPERTIES aProperty);
	int setProperty(CAPTURE_PROPERTIES aProperty, float aValue, int aAuto);
	int getProperty(CAPTURE_PROPERTIES aProperty, float& aValue, int& aAuto, float& min, float& max);
	std::vector<CAPTURE_PROPERTIES> getPropertyList();
	BOOL isFormatSupported(REFGUID aSubtype) const;
	HRESULT getFormat(DWORD aIndex, GUID* aSubtype) const;
	HRESULT setConversionFunction(REFGUID aSubtype);
	HRESULT setVideoType(SimpleFormat fomat, IMFMediaType* aType);
	int isMediaOk(IMFMediaType* aType, unsigned int aIndex);
	int scanMediaTypes(unsigned int aWidth, unsigned int aHeight);
	std::vector<resolution> getSupportedResolutions();

	HRESULT initCapture(const struct SimpleCapParams* aParams, unsigned int aOptions);

	bool changeResolution(SimpleFormat format, size_t width, size_t height);

	void deinitCapture();

	std::wstring name() const;
	std::string cname() const;

	size_t getCaptureImage(char**buffer, size_t* stride, size_t* height);

  private:
	static std::wstring updatename(IMFActivate* device);

	IMFActivate* get(ChooseDeviceParam& param);

  public:
	int mRedoFromStart;
	int gDoCapture;
	int mErrorLine;
	int mErrorCode;
	int gOptions;
	struct SimpleCapParams gParams;

  private:
	long mRefCount; // Reference count.
	std::mutex mutex;

	IMFSourceReader* mReader;
	IMFMediaSource* mSource;

	LONG mDefaultStride;
	IMAGE_TRANSFORM_FN mConvertFn; // Function to convert the video to RGB32

	std::vector<char> mCaptureBuffer;
	size_t mCaptureBufferScanline;
	size_t mCaptureBufferWidth;
	size_t mCaptureBufferHeight;

	std::vector<unsigned int> mBadIndex;
	unsigned int mBadIndices;
	unsigned int mMaxBadIndices;
	unsigned int mUsedIndex;

  private:
	std::vector<resolution> resolutions;
	std::wstring wcname;
};
