#include <windows.h>
#include "escapi.h"

countCaptureDevicesProc countCaptureDevices;
getCaptureDeviceIdsProc getCaptureDeviceIds;
getCaptureSupportedResolutionsProc getCaptureSupportedResolutions;
initCaptureProc initCapture;
deinitCaptureProc deinitCapture;
doCaptureProc doCapture;
isCaptureDoneProc isCaptureDone;
getCaptureDeviceNameProc getCaptureDeviceName;
ESCAPIVersionProc ESCAPIVersion;
getCapturePropertyValueProc getCapturePropertyValue;
getCapturePropertyAutoProc getCapturePropertyAuto;
setCapturePropertyProc setCaptureProperty;
getCaptureErrorLineProc getCaptureErrorLine;
getCaptureErrorCodeProc getCaptureErrorCode;
initCaptureWithOptionsProc initCaptureWithOptions;

/* Internal: initialize COM */
typedef void (*initCOMProc)(void);
initCOMProc initCOM;

size_t setupESCAPI(void)
{
	/* Load DLL dynamically */
	HMODULE capdll = LoadLibraryA("escapi.dll");
	if (capdll == nullptr)
		return 0;

	/* Fetch function entry points */
	countCaptureDevices     = reinterpret_cast<countCaptureDevicesProc>(GetProcAddress(capdll, "countCaptureDevices"));
	getCaptureDeviceIds     = reinterpret_cast<getCaptureDeviceIdsProc>(GetProcAddress(capdll, "getCaptureDeviceIds"));
	getCaptureSupportedResolutions = reinterpret_cast<getCaptureSupportedResolutionsProc>(
	    GetProcAddress(capdll, "getCaptureSupportedResolutions"));
	initCapture             = reinterpret_cast<initCaptureProc>(GetProcAddress(capdll, "initCapture"));
	deinitCapture           = reinterpret_cast<deinitCaptureProc>(GetProcAddress(capdll, "deinitCapture"));
	doCapture               = reinterpret_cast<doCaptureProc>(GetProcAddress(capdll, "doCapture"));
	isCaptureDone           = reinterpret_cast<isCaptureDoneProc>(GetProcAddress(capdll, "isCaptureDone"));
	initCOM                 = reinterpret_cast<initCOMProc>(GetProcAddress(capdll, "initCOM"));
	getCaptureDeviceName    = reinterpret_cast<getCaptureDeviceNameProc>(GetProcAddress(capdll, "getCaptureDeviceName"));
	ESCAPIVersion           = reinterpret_cast<ESCAPIVersionProc>(GetProcAddress(capdll, "ESCAPIVersion"));
	getCapturePropertyValue = reinterpret_cast<getCapturePropertyValueProc>(GetProcAddress(capdll, "getCapturePropertyValue"));
	getCapturePropertyAuto  = reinterpret_cast<getCapturePropertyAutoProc>(GetProcAddress(capdll, "getCapturePropertyAuto"));
	setCaptureProperty      = reinterpret_cast<setCapturePropertyProc>(GetProcAddress(capdll, "setCaptureProperty"));
	getCaptureErrorLine     = reinterpret_cast<getCaptureErrorLineProc>(GetProcAddress(capdll, "getCaptureErrorLine"));
	getCaptureErrorCode     = reinterpret_cast<getCaptureErrorCodeProc>(GetProcAddress(capdll, "getCaptureErrorCode"));
	initCaptureWithOptions  = reinterpret_cast<initCaptureWithOptionsProc>(GetProcAddress(capdll, "initCaptureWithOptions"));

	/* Check that we got all the entry points */
	if ((initCOM == nullptr) || (ESCAPIVersion == nullptr) || (getCaptureDeviceName == nullptr) ||
	    (getCaptureDeviceIds == nullptr) || (getCaptureSupportedResolutions == nullptr) ||
	    (countCaptureDevices == nullptr) || (initCapture == nullptr) ||
	    (deinitCapture == nullptr) || (doCapture == nullptr) || (isCaptureDone == nullptr) ||
	    (getCapturePropertyValue == nullptr) || (getCapturePropertyAuto == nullptr) ||
	    (setCaptureProperty == nullptr) || (getCaptureErrorLine == nullptr) ||
	    (getCaptureErrorCode == nullptr) || (initCaptureWithOptions == nullptr))
		return 0;

	/* Verify DLL version is at least what we want */
	if (ESCAPIVersion() < 0x301)
		return 0;

	/* Initialize COM.. */
	initCOM();

	/* and return the number of capture devices found. */
	return countCaptureDevices();
}
