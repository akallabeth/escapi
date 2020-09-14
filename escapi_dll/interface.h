#pragma once

#include <Windows.h>

#include <map>
#include <memory>
#include <escapi.h>
#include "capture.h"

class EscAPI
{
  public:
	static void SetHotplugCallback(hotplug_event_t fkt, void* context);
	static HRESULT InitDevice(size_t device, const struct SimpleCapParams* aParams,
	                          unsigned int opts);

	static size_t GetSupportedFormatsAndResolutions(size_t device, SimpleFormat* formats, size_t* widths, size_t* heights,
	                                      size_t count);

	static void CleanupDevice(size_t device);
	static size_t CountCaptureDevices();
	static size_t GetCaptureDeviceIds(size_t* buffer, size_t count);
	static size_t GetCaptureDeviceName(size_t deviceno, char* namebuffer, size_t bufferlength);
	static size_t GetCaptureDeviceNameW(size_t deviceno, wchar_t* namebuffer, size_t bufferlength);
	static int GetErrorCode(size_t device);
	static int GetErrorLine(size_t device);
	static size_t GetPropertyList(size_t device, CAPTURE_PROPETIES* properties, size_t count);
	static float GetProperty(size_t device, CAPTURE_PROPETIES prop);
	static float GetPropertyMin(size_t device, CAPTURE_PROPETIES prop);
	static float GetPropertyMax(size_t device, CAPTURE_PROPETIES prop);
	static int GetPropertyAuto(size_t device, CAPTURE_PROPETIES prop);
	static int SetProperty(size_t device, CAPTURE_PROPETIES prop, float value, int autoval);
	static void DoCapture(size_t deviceno);
	static int IsCaptureDone(size_t deviceno);

  private:
	static bool CheckForFail(size_t aDevice);
	static bool update();
	static bool getDevice(size_t id, CaptureClass** device);
	static SSIZE_T contains(const std::map<size_t, std::unique_ptr<CaptureClass>>& map,
	                        CaptureClass& device);

  private:
	static hotplug_event_t sHotplugEvent;
	static void* sHotplugConext;
	static std::map<size_t, std::unique_ptr<CaptureClass>> sDeviceList;
};
