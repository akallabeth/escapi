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
#include <memory>
#include <random>
#include <limits>

std::map<size_t, std::unique_ptr<CaptureClass>> EscAPI::sDeviceList;
hotplug_event_t EscAPI::sHotplugEvent = nullptr;
void* EscAPI::sHotplugConext = nullptr;

void EscAPI::DoCapture(size_t deviceno)
{
	CaptureClass* dev;
	if (getDevice(deviceno, &dev))
	{
		dev->gDoCapture = -1;
	}
}

int EscAPI::IsCaptureDone(size_t deviceno)
{
	if (!CheckForFail(deviceno))
		return 0;

	CaptureClass* dev;
	if (getDevice(deviceno, &dev))
	{
		return dev->gDoCapture == 1 ? 1 : 0;
	}

	return 0;
}

size_t EscAPI::GetCaptureImage(size_t deviceno, char **buffer, size_t *stride, size_t *height) {
	if (!CheckForFail(deviceno))
		return 0;

	CaptureClass* dev;
	if (getDevice(deviceno, &dev))
	{
		return dev->getCaptureImage(buffer, stride, height);
	}

	return 0;
}

void EscAPI::CleanupDevice(size_t aDevice)
{
	CaptureClass* dev;
	if (getDevice(aDevice, &dev))
	{
		dev->deinitCapture();
	}
}

void EscAPI::SetHotplugCallback(hotplug_event_t fkt, void* context)
{
	sHotplugEvent = fkt;
	sHotplugConext = context;
}

HRESULT EscAPI::InitDevice(size_t aDevice, const struct SimpleCapParams* aParams,
                           unsigned int aOptions)
{
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return E_FAIL;
	}
	return dev->initCapture(aParams, aOptions);
}

size_t EscAPI::GetSupportedFormatsAndResolutions(size_t device, SimpleFormat* formats, size_t* widths, size_t* heights, size_t count)
{
	CaptureClass* dev;
	if (!getDevice(device, &dev))
		return 0;

	std::vector<CaptureClass::resolution> resolutions = dev->getSupportedResolutions();
	size_t used = std::min<size_t>(count, resolutions.size() * 4);
	if (!formats || !widths || !heights || (count == 0))
		return resolutions.size() * 4;

	for (size_t x = 0; (x < used) && !resolutions.empty();)
	{
		CaptureClass::resolution res = resolutions.back();
		resolutions.pop_back();
		for (size_t y=H264; (y<=RGB32)&&(x < used); y++) {
			if (y == I420)
				continue;
			if (y == H264)
				continue;
			if (y == MJPG)
				continue;

			formats[x] = static_cast<SimpleFormat>(y);
			widths[x] = res.w;
			heights[x] = res.h;
			x++;
		}
	}
	return used;
}

size_t EscAPI::CountCaptureDevices()
{
	update();
	return sDeviceList.size();
}

size_t EscAPI::GetCaptureDeviceIds(size_t *buffer, size_t count)
{
	update();
	if (!buffer)
		return sDeviceList.size();
	size_t pos = 0;
	size_t used = std::min<size_t>(count, sDeviceList.size());
	for (auto& it : sDeviceList)
	{
		if (pos >= used)
			break;
		buffer[pos++] = it.first;
	}

	return used;
}

size_t EscAPI::GetCaptureDeviceName(size_t aDevice, char* aNamebuffer, size_t aBufferlength)
{
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}

	std::string name = dev->cname();
	if (!aNamebuffer && (aBufferlength == 0))
		return name.size();

	if (aBufferlength < 1)
		return 0;

	size_t used = std::min<size_t>(name.size(), aBufferlength - 1);
	strcpy_s(aNamebuffer, used + 1, name.c_str());
	return used;
}

size_t EscAPI::GetCaptureDeviceNameW(size_t aDevice, wchar_t* aNamebuffer, size_t aBufferlength)
{
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}

	std::wstring name = dev->name();
	if (!aNamebuffer && (aBufferlength == 0))
		return name.size();

	if (aBufferlength < 1)
		return 0;

	size_t used = std::min<size_t>(name.size(), aBufferlength - 1);
	wcscpy_s(aNamebuffer, used + 1, name.c_str());
	return used;
}

bool EscAPI::CheckForFail(size_t aDevice)
{
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return false;
	}
	if (dev->mRedoFromStart)
	{
		dev->mRedoFromStart = 0;
		dev->deinitCapture();

		HRESULT hr = dev->initCapture(&dev->gParams, dev->gOptions);
		if (FAILED(hr))
			return false;
	}

	return true;
}

bool EscAPI::update()
{
	ChooseDeviceParam param = {};
	if (!CaptureClass::getall(param))
		return false;

	std::vector<size_t> available;
	std::vector<size_t> added;
	for (size_t x = 0; x < param.mCount; x++)
	{
		std::unique_ptr<CaptureClass> device(new CaptureClass(param.mDevices[x]));

		auto idx = contains(sDeviceList, *device);
		if (idx < 0)
		{
			const size_t limit = MAXSSIZE_T;
			std::default_random_engine generator;
			std::uniform_int_distribution<size_t> distribution(1, limit);
			size_t sDeviceCount;
			do {
				sDeviceCount = distribution(generator);
			} while(sDeviceList.find(sDeviceCount) != sDeviceList.end());

			sDeviceList.emplace(std::make_pair<size_t, std::unique_ptr<CaptureClass>>(
			    std::move(sDeviceCount), std::move(device)));
			added.push_back(sDeviceCount);
			available.push_back(sDeviceCount);
		}
		else
		{
			available.push_back(idx);
		}
	}

	std::vector<size_t> removed;
	for (auto& it : sDeviceList)
	{
		bool found = false;
		for (auto ix = available.begin(); ix != available.end(); ix++)
		{
			if (it.first == *ix)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			removed.push_back(it.first);
		}
	}

	for (auto it : removed)
	{
		sDeviceList.erase(it);
	}

	if (sHotplugEvent)
	{
		for (auto it : removed)
		{
			sHotplugEvent(sHotplugConext, it, false);
		}
		for (auto it : added)
		{
			sHotplugEvent(sHotplugConext, it, true);
		}
	}
	return true;
}

int EscAPI::GetErrorCode(size_t aDevice)
{
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	return dev->mErrorCode;
}

int EscAPI::GetErrorLine(size_t aDevice)
{
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	return dev->mErrorLine;
}

size_t EscAPI::GetPropertyList(size_t aDevice, CAPTURE_PROPERTIES *properties, size_t count)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val;
	int autoval;
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	std::vector<CAPTURE_PROPERTIES> list = dev->getPropertyList();
	if (properties)
	{
		size_t cpy = std::min<size_t>(count, list.size());
		memcpy(properties, list.data(), cpy * sizeof(CAPTURE_PROPERTIES));
	}
	return list.size();
}

float EscAPI::GetProperty(size_t aDevice, CAPTURE_PROPERTIES aProp)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val, min, max;
	int autoval;
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	dev->getProperty(aProp, val, autoval, min, max);
	return val;
}

float EscAPI::GetPropertyMin(size_t aDevice, CAPTURE_PROPERTIES aProp)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val, min, max;
	int autoval;
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	dev->getProperty(aProp, val, autoval, min, max);
	return min;
}

float EscAPI::GetPropertyMax(size_t aDevice, CAPTURE_PROPERTIES aProp)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val, min, max;
	int autoval;
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	dev->getProperty(aProp, val, autoval, min, max);
	return max;
}


int EscAPI::GetPropertyAuto(size_t aDevice, CAPTURE_PROPERTIES aProp)
{
	if (!CheckForFail(aDevice))
		return 0;
	float val, min, max;
	int autoval;
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	dev->getProperty(aProp, val, autoval, min, max);
	return autoval;
}

int EscAPI::SetProperty(size_t aDevice, CAPTURE_PROPERTIES aProp, float aValue, int aAutoval)
{
	if (!CheckForFail(aDevice))
		return 0;
	CaptureClass* dev;
	if (!getDevice(aDevice, &dev))
	{
		return 0;
	}
	return dev->setProperty(aProp, aValue, aAutoval);
}

bool EscAPI::getDevice(size_t device, CaptureClass** dev)
{
	if (!dev)
		return false;

	update();

	auto it = sDeviceList.find(device);
	if (it == sDeviceList.end())
		return false;
	*dev = it->second.get();
	return true;
}

SSIZE_T EscAPI::contains(const std::map<size_t, std::unique_ptr<CaptureClass>>& map,
                         CaptureClass& device)
{
	const std::wstring& cmp = device.name();
	for (auto& it : map)
	{
		const std::wstring& cur = it.second->name();
		if (cmp.compare(cur) == 0)
		{
			return it.first;
		}
	}
	return -1;
}
