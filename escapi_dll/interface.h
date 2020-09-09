#pragma once

#include <Windows.h>

#include <vector>
#include "capture.h"

class EscAPI {
public:
    static HRESULT InitDevice(size_t device, const struct SimpleCapParams* aParams, unsigned int opts);
    static void CleanupDevice(size_t device);
    static size_t CountCaptureDevices();
    static void GetCaptureDeviceName(size_t deviceno, char * namebuffer, int bufferlength);
    static int GetErrorCode(size_t device);
    static int GetErrorLine(size_t device);
    static float GetProperty(size_t device, int prop);
    static int GetPropertyAuto(size_t device, int prop);
    static int SetProperty(size_t device, int prop, float value, int autoval);
    static void DoCapture(size_t deviceno);
    static int IsCaptureDone(size_t deviceno);

private:
    static bool CheckForFail(size_t aDevice);

private:
    static std::vector<CaptureClass> sDeviceList;
    static CaptureClass& getDevice(size_t device);
};
