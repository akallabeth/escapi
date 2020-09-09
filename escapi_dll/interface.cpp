#include <initguid.h>
#include <cguid.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#define ESCAPI_DEFINITIONS_ONLY
#include "escapi.h"

#include "interface.h"
#include "conversion.h"
#include "capture.h"
#include "scopedrelease.h"
#include "choosedeviceparam.h"

#include <vector>

std::vector<CaptureClass> EscAPI::sDeviceList;

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
    if (aDevice < sDeviceList.size())
	{
        sDeviceList.erase(sDeviceList.begin() + aDevice);
	}
}
HRESULT EscAPI::InitDevice(size_t aDevice, const struct SimpleCapParams *aParams, unsigned int aOptions)
{
    CleanupDevice(aDevice);

    sDeviceList.push_back(CaptureClass());
    HRESULT hr = getDevice(aDevice).initCapture(aDevice, aParams, aOptions);
	if (FAILED(hr))
        CleanupDevice(aDevice);

	return hr;
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
	IMFAttributes *attributes = NULL;
	hr = MFCreateAttributes(&attributes, 1);
	ScopedRelease<IMFAttributes> attributes_s(attributes);

    if (FAILED(hr))
        return 0;

	hr = attributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
		);
    if (FAILED(hr))
        return 0;

    ChooseDeviceParam param = { };
	hr = MFEnumDeviceSources(attributes, &param.mDevices, &param.mCount);

    if (FAILED(hr))
        return 0;

	return param.mCount;
}

void EscAPI::GetCaptureDeviceName(size_t aDevice, char * aNamebuffer, int aBufferlength)
{
	int i;
	if (!aNamebuffer || aBufferlength <= 0)
		return;

	aNamebuffer[0] = 0;

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (FAILED(hr))
        return;

	hr = MFStartup(MF_VERSION);

    if (FAILED(hr))
        return;

	// choose device
	IMFAttributes *attributes = NULL;
	hr = MFCreateAttributes(&attributes, 1);
	ScopedRelease<IMFAttributes> attributes_s(attributes);

    if (FAILED(hr))
        return;

	hr = attributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
		);

    if (FAILED(hr))
        return;

    ChooseDeviceParam param = { };
	hr = MFEnumDeviceSources(attributes, &param.mDevices, &param.mCount);

    if (FAILED(hr))
        return;

    if (aDevice < param.mCount)
	{
		WCHAR *name = 0;
		UINT32 namelen = 255;
		hr = param.mDevices[aDevice]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&name,
			&namelen
			);
		if (SUCCEEDED(hr) && name)
		{
			i = 0;
			while (i < aBufferlength - 1 && i < (signed)namelen && name[i] != 0)
			{
				aNamebuffer[i] = (char)name[i];
				i++;
			}
			aNamebuffer[i] = 0;

			CoTaskMemFree(name);
		}
	}
}

bool EscAPI::CheckForFail(size_t aDevice)
{
    if (sDeviceList.size() <= aDevice)
        return false;

    if (getDevice(aDevice).mRedoFromStart)
	{
        CaptureClass& dev = getDevice(aDevice);
        dev.mRedoFromStart = 0;
        dev.deinitCapture();

        HRESULT hr = dev.initCapture(aDevice, &dev.gParams, dev.gOptions);
		if (FAILED(hr))
		{
            CleanupDevice(aDevice);
            return false;
		}
	}

    return true;
}


int EscAPI::GetErrorCode(size_t aDevice)
{
    if (sDeviceList.size() <= aDevice)
		return 0;
    return getDevice(aDevice).mErrorCode;
}

int EscAPI::GetErrorLine(size_t aDevice)
{
    if (sDeviceList.size() <= aDevice)
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

CaptureClass &EscAPI::getDevice(size_t device)
{
    return sDeviceList.at(device);
}
