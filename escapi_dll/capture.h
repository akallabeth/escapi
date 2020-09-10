#pragma once

#include <initguid.h>
#include <cguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <vector>

#include "conversion.h"

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
	CaptureClass(size_t index);
	virtual ~CaptureClass();

	STDMETHODIMP QueryInterface(REFIID aRiid, void** aPpv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP OnReadSample(HRESULT aStatus, DWORD aStreamIndex, DWORD aStreamFlags,
	                          LONGLONG aTimestamp, IMFSample* aSample);
	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent*);
	STDMETHODIMP OnFlush(DWORD);
	int escapiPropToMFProp(int aProperty);
	int setProperty(int aProperty, float aValue, int aAuto);
	int getProperty(int aProperty, float& aValue, int& aAuto);
	BOOL isFormatSupported(REFGUID aSubtype) const;
	HRESULT getFormat(DWORD aIndex, GUID* aSubtype) const;
	HRESULT setConversionFunction(REFGUID aSubtype);
	HRESULT setVideoType(IMFMediaType* aType);
	int isMediaOk(IMFMediaType* aType, unsigned int aIndex);
	int scanMediaTypes(unsigned int aWidth, unsigned int aHeight);
	std::vector<resolution> getSupportedResolutions();

	HRESULT initCapture(const struct SimpleCapParams* aParams, unsigned int aOptions);

	bool changeResolution(size_t width, size_t height);

	void deinitCapture();

  public:
	int mRedoFromStart;
	int gDoCapture;
	int mErrorLine;
	int mErrorCode;
	int gOptions;
	struct SimpleCapParams gParams;

  private:
	size_t deviceindex;
	long mRefCount; // Reference count.
	CRITICAL_SECTION mCritsec;

	IMFSourceReader* mReader;
	IMFMediaSource* mSource;

	LONG mDefaultStride;
	IMAGE_TRANSFORM_FN mConvertFn; // Function to convert the video to RGB32

	unsigned int* mCaptureBuffer;
	unsigned int mCaptureBufferWidth, mCaptureBufferHeight;

	std::vector<unsigned int> mBadIndex;
	unsigned int mBadIndices;
	unsigned int mMaxBadIndices;
	unsigned int mUsedIndex;

  private:
	std::vector<resolution> resolutions;
};
