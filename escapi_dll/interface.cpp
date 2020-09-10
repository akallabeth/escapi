#include <initguid.h>
#include <cguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <algorithm>

#define ESCAPI_DEFINITIONS_ONLY
#include "escapi.h"

#include "interface.h"
#include "conversion.h"
#include "capture.h"
#include "scopedrelease.h"
#include "choosedeviceparam.h"

#include <map>

std::map<size_t, CaptureClass> EscAPI::sDeviceList;
size_t EscAPI::sDeviceCount = 23;

void EscAPI::DoCapture(size_t deviceno)
{
	if (CheckForFail(deviceno))
		getDevice(deviceno).gDoCapture = -1;
}

int EscAPI::IsCaptureDone(size_t deviceno)
{
	if (!CheckForFail(deviceno))
		return 0;

	if (getDevice(deviceno).gDoCapture == 1)
		return 1;
	return 0;
}

void EscAPI::CleanupDevice(size_t aDevice)
{
    sDeviceList.erase(aDevice);
}

HRESULT EscAPI::InitDevice(size_t aDevice, const struct SimpleCapParams* aParams,
                           unsigned int aOptions)
{
	return getDevice(aDevice).initCapture(aParams, aOptions);
}

size_t EscAPI::GetSupportedResolutions(size_t device, size_t* widths, size_t* heights, size_t count)
{
	if (sDeviceList.find(device) == sDeviceList.end())
		return 0;
	CaptureClass& dev = sDeviceList.at(device);
	std::vector<CaptureClass::resolution> resolutions = dev.getSupportedResolutions();
	size_t used = std::min<size_t>(count, resolutions.size());
	if (!widths || !heights || (count == 0))
		return resolutions.size();

	for (size_t x = 0; x < used; x++)
	{
		CaptureClass::resolution& res = resolutions.at(x);
		widths[x] = res.w;
		heights[x] = res.h;
	}
	return used;
}

size_t EscAPI::CountCaptureDevices()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (FAILED(hr))
		return 0;

	hr = MFStartup(MF_VERSION);

	if (FAILED(hr))
		return 0;

	// choose device
	IMFAttributes* attributes = NULL;
	hr = MFCreateAttributes(&attributes, 1);
	ScopedRelease<IMFAttributes> attributes_s(attributes);

	if (FAILED(hr))
		return 0;

	hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
	                         MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr))
		return 0;

	ChooseDeviceParam param = {};
	hr = MFEnumDeviceSources(attributes, &param.mDevices, &param.mCount);

	if (FAILED(hr))
		return 0;

	sDeviceList.clear();
	for (size_t x = 0; x < param.mCount; x++)
	{
		std::pair<size_t, CaptureClass> pair =
		    std::pair<size_t, CaptureClass>(sDeviceCount++, CaptureClass(x));

		sDeviceList.insert(pair);
	}

	return param.mCount;
}

size_t EscAPI::GetCaptureDeviceIds(size_t *buffer, size_t count)
{
	if (!buffer)
		return sDeviceList.size();
	size_t pos = 0;
	size_t used = std::min<size_t>(count, sDeviceList.size());
	for (std::map<size_t, CaptureClass>::const_iterator it = sDeviceList.begin(); it != sDeviceList.end(); it++)
	{
		if (pos >= used)
			break;
		buffer[pos++] = it->first;
	}

	return used;
}

size_t EscAPI::GetCaptureDeviceName(size_t aDevice, char* aNamebuffer, size_t aBufferlength)
{
	if (sDeviceList.find(aDevice) == sDeviceList.end())
		return 0;
	if (aBufferlength < 1)
		return 0;
	std::string name = getDevice(aDevice).cname();
	size_t used = std::min<size_t>(name.size(), aBufferlength - 1);
	strcpy_s(aNamebuffer, used + 1, name.c_str());
	return used;
}

size_t EscAPI::GetCaptureDeviceNameW(size_t aDevice, wchar_t* aNamebuffer, size_t aBufferlength)
{
	if (sDeviceList.find(aDevice) == sDeviceList.end())
		return 0;
	if (aBufferlength < 1)
		return 0;
	std::wstring name = getDevice(aDevice).name();
	size_t used = std::min<size_t>(name.size(), aBufferlength - 1);
	wcscpy_s(aNamebuffer, used + 1, name.c_str());
	return used;
}

bool EscAPI::CheckForFail(size_t aDevice)
{
	if (sDeviceList.find(aDevice) == sDeviceList.end())
		return false;

	if (getDevice(aDevice).mRedoFromStart)
	{
		CaptureClass& dev = getDevice(aDevice);
		dev.mRedoFromStart = 0;
		dev.deinitCapture();

		HRESULT hr = dev.initCapture(&dev.gParams, dev.gOptions);
		if (FAILED(hr))
			return false;
	}

	return true;
}

int EscAPI::GetErrorCode(size_t aDevice)
{
	if (sDeviceList.find(aDevice) == sDeviceList.end())
		return 0;
	return getDevice(aDevice).mErrorCode;
}

int EscAPI::GetErrorLine(size_t aDevice)
{
	if (sDeviceList.find(aDevice) == sDeviceList.end())
		return 0;
	return getDevice(aDevice).mErrorLine;
}

float EscAPI::GetProperty(size_t aDevice, int aProp)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val;
	int autoval;
	getDevice(aDevice).getProperty(aProp, val, autoval);
	return val;
}

int EscAPI::GetPropertyAuto(size_t aDevice, int aProp)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val;
	int autoval;
	getDevice(aDevice).getProperty(aProp, val, autoval);
	return autoval;
}

int EscAPI::SetProperty(size_t aDevice, int aProp, float aValue, int aAutoval)
{
	if (!CheckForFail(aDevice))
		return 0;
	return getDevice(aDevice).setProperty(aProp, aValue, aAutoval);
}

CaptureClass& EscAPI::getDevice(size_t device)
{
	return sDeviceList.at(device);
}
