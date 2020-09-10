#include "windows.h"
#define ESCAPI_DEFINITIONS_ONLY
#include "escapi.h"

#include "interface.h"

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

extern "C" void __declspec(dllexport)
	getCaptureDeviceName(size_t deviceno, char* namebuffer, int bufferlength)
{
	EscAPI::GetCaptureDeviceName(deviceno, namebuffer, bufferlength);
}

extern "C" int __declspec(dllexport) ESCAPIDLLVersion()
{
	return 0x200; // due to mess up, earlier programs check for exact version; this needs to stay
	              // constant
}

extern "C" int __declspec(dllexport) ESCAPIVersion()
{
	return 0x301; // ...and let's hope this one works better
}

extern "C" size_t __declspec(dllexport) countCaptureDevices()
{
	return EscAPI::CountCaptureDevices();
}

extern "C" size_t __declspec(dllexport) getCaptureDeviceIds(size_t* buffer, size_t count)
{
	return EscAPI::GetCaptureDeviceIds(buffer, count);
}

extern "C" size_t __declspec(dllexport)
    getCaptureSupportedResolutions(size_t deviceno, size_t* widths, size_t* heights, size_t count)
{
	return EscAPI::GetSupportedResolutions(deviceno, widths, heights, count);
}

extern "C" void __declspec(dllexport) initCOM()
{
	CoInitialize(NULL);
}

extern "C" int __declspec(dllexport)
	initCapture(size_t deviceno, struct SimpleCapParams* aParams)
{
	if (aParams == NULL || aParams->mHeight <= 0 || aParams->mWidth <= 0 ||
	    aParams->mTargetBuf == 0)
		return 0;
	if (FAILED(EscAPI::InitDevice(deviceno, aParams, 0)))
		return 0;
	return 1;
}

extern "C" void __declspec(dllexport) deinitCapture(size_t deviceno)
{
	EscAPI::CleanupDevice(deviceno);
}

extern "C" void __declspec(dllexport) doCapture(size_t deviceno)
{
	EscAPI::DoCapture(deviceno);
}

extern "C" int __declspec(dllexport) isCaptureDone(size_t deviceno)
{
	return EscAPI::IsCaptureDone(deviceno);
}

extern "C" int __declspec(dllexport) getCaptureErrorLine(size_t deviceno)
{
	return EscAPI::GetErrorLine(deviceno);
}

extern "C" int __declspec(dllexport) getCaptureErrorCode(size_t deviceno)
{
	return EscAPI::GetErrorCode(deviceno);
}

extern "C" float __declspec(dllexport) getCapturePropertyValue(size_t deviceno, int prop)
{
	return EscAPI::GetProperty(deviceno, prop);
}

extern "C" int __declspec(dllexport) getCapturePropertyAuto(size_t deviceno, int prop)
{
	return EscAPI::GetPropertyAuto(deviceno, prop);
}

extern "C" int __declspec(dllexport)
	setCaptureProperty(size_t deviceno, int prop, float value, int autoval)
{
	return EscAPI::SetProperty(deviceno, prop, value, autoval);
}

extern "C" int __declspec(dllexport)
    initCaptureWithOptions(size_t deviceno, struct SimpleCapParams* aParams,
                           unsigned int aOptions)
{
	if (aParams == NULL || aParams->mHeight <= 0 || aParams->mWidth <= 0 ||
	    aParams->mTargetBuf == 0)
		return 0;
	if ((aOptions & CAPTURE_OPTIONS_MASK) != aOptions)
		return 0;
	if (FAILED(EscAPI::InitDevice(deviceno, aParams, aOptions)))
		return 0;
	return 1;
}
