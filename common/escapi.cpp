#include <windows.h>
#include "escapi.h"

setCaptureDeviceHotplugFunctionProc setCaptureDeviceHotplugFunction;
countCaptureDevicesProc countCaptureDevices;
getCaptureDeviceIdsProc getCaptureDeviceIds;
getCaptureSupportedFormatsAndResolutionsProc getCaptureSupportedFormatsAndResolutions;
initCaptureProc initCapture;
deinitCaptureProc deinitCapture;
doCaptureProc doCapture;
isCaptureDoneProc isCaptureDone;
getCaptureImageProc getCaptureImage;
getCaptureDeviceNameProc getCaptureDeviceName;
getCaptureDeviceNameWProc getCaptureDeviceNameW;
ESCAPIVersionProc ESCAPIVersion;
getCapturePropertyListProc getCapturePropertyList;
getCapturePropertyValueProc getCapturePropertyValue;
getCapturePropertyMinProc getCapturePropertyMin;
getCapturePropertyMaxProc getCapturePropertyMax;
getCapturePropertyAutoProc getCapturePropertyAuto;
setCapturePropertyProc setCaptureProperty;
getCaptureErrorLineProc getCaptureErrorLine;
getCaptureErrorCodeProc getCaptureErrorCode;
initCaptureWithOptionsProc initCaptureWithOptions;

/* Internal: initialize COM */
typedef void (*initCOMProc)(void);
initCOMProc initCOM;

size_t setupESCAPI(hotplug_event_t fkt, void* context)
{
	/* Load DLL dynamically */
	HMODULE capdll = LoadLibraryA("escapi.dll");
	if (capdll == nullptr)
		return 0;

	/* Fetch function entry points */
	ESCAPIVersion           = reinterpret_cast<ESCAPIVersionProc>(GetProcAddress(capdll, "ESCAPIVersion"));
	if (!ESCAPIVersion)
		return 0;

	/* Verify DLL version is at least what we want */
	if (ESCAPIVersion() < 0x301)
		return 0;

	countCaptureDevices     = reinterpret_cast<countCaptureDevicesProc>(GetProcAddress(capdll, "countCaptureDevices"));
	if (!countCaptureDevices)
		return 0;

	setCaptureDeviceHotplugFunction = reinterpret_cast<setCaptureDeviceHotplugFunctionProc>(
		GetProcAddress(capdll, "setCaptureDeviceHotplugFunction"));
	if (!setCaptureDeviceHotplugFunction)
		return 0;

	getCaptureDeviceIds     = reinterpret_cast<getCaptureDeviceIdsProc>(GetProcAddress(capdll, "getCaptureDeviceIds"));
	if (!getCaptureDeviceIds)
		return 0;

	getCaptureSupportedFormatsAndResolutions = reinterpret_cast<getCaptureSupportedFormatsAndResolutionsProc>(
		GetProcAddress(capdll, "getCaptureSupportedFormatsAndResolutions"));
	if (!getCaptureSupportedFormatsAndResolutions)
		return 0;

	initCapture             = reinterpret_cast<initCaptureProc>(GetProcAddress(capdll, "initCapture"));
	if (!initCapture)
		return 0;

	deinitCapture           = reinterpret_cast<deinitCaptureProc>(GetProcAddress(capdll, "deinitCapture"));
	if (!deinitCapture)
		return 0;

	doCapture               = reinterpret_cast<doCaptureProc>(GetProcAddress(capdll, "doCapture"));
	if (!doCapture)
		return 0;

	isCaptureDone           = reinterpret_cast<isCaptureDoneProc>(GetProcAddress(capdll, "isCaptureDone"));
	if (!isCaptureDone)
		return 0;

	getCaptureImage         = reinterpret_cast<getCaptureImageProc>(GetProcAddress(capdll, "getCaptureImage"));
	if (!getCaptureImage)
		return 0;

	initCOM                 = reinterpret_cast<initCOMProc>(GetProcAddress(capdll, "initCOM"));
	if (!initCOM)
		return 0;

	getCaptureDeviceName    = reinterpret_cast<getCaptureDeviceNameProc>(GetProcAddress(capdll, "getCaptureDeviceName"));
	if (!getCaptureDeviceName)
		return 0;

	getCaptureDeviceNameW = reinterpret_cast<getCaptureDeviceNameWProc>(
		GetProcAddress(capdll, "getCaptureDeviceNameW"));
	if (!getCaptureDeviceNameW)
		return 0;

	getCapturePropertyList = reinterpret_cast<getCapturePropertyListProc>(GetProcAddress(capdll, "getCapturePropertyList"));
	if (!getCapturePropertyList)
		return 0;

	getCapturePropertyValue = reinterpret_cast<getCapturePropertyValueProc>(GetProcAddress(capdll, "getCapturePropertyValue"));
	if (!getCapturePropertyValue)
		return 0;

	getCapturePropertyMin = reinterpret_cast<getCapturePropertyMinProc>(GetProcAddress(capdll, "getCapturePropertyMin"));
	if (!getCapturePropertyMin)
		return 0;

	getCapturePropertyMax = reinterpret_cast<getCapturePropertyMaxProc>(GetProcAddress(capdll, "getCapturePropertyMax"));
	if (!getCapturePropertyMax)
		return 0;

	getCapturePropertyAuto  = reinterpret_cast<getCapturePropertyAutoProc>(GetProcAddress(capdll, "getCapturePropertyAuto"));
	if (!getCapturePropertyAuto)
		return 0;

	setCaptureProperty      = reinterpret_cast<setCapturePropertyProc>(GetProcAddress(capdll, "setCaptureProperty"));
	if (!setCaptureProperty)
		return 0;

	getCaptureErrorLine     = reinterpret_cast<getCaptureErrorLineProc>(GetProcAddress(capdll, "getCaptureErrorLine"));
	if (!getCaptureErrorLine)
		return 0;

	getCaptureErrorCode     = reinterpret_cast<getCaptureErrorCodeProc>(GetProcAddress(capdll, "getCaptureErrorCode"));
	if (!getCaptureErrorCode)
		return 0;

	initCaptureWithOptions  = reinterpret_cast<initCaptureWithOptionsProc>(GetProcAddress(capdll, "initCaptureWithOptions"));
	if (!initCaptureWithOptions)
		return 0;

	/* Initialize COM.. */
	initCOM();

	setCaptureDeviceHotplugFunction(fkt, context);

	/* and return the number of capture devices found. */
	return countCaptureDevices();
}
