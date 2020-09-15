#pragma once
/* Extremely Simple Capture API */

enum SimpleFormat
{
	H264 = 1,
	MJPG = 2,
	YUY2 = 3,
	NV12 = 4,
	I420 = 5,
	RGB24 = 6,
	RGB32 = 7
};

struct SimpleCapParams
{
	SimpleFormat Format;

	/* Buffer width */
	size_t mWidth;
	/* Buffer height */
	size_t mHeight;
};

enum CAPTURE_PROPERTIES
{
	CAPTURE_BRIGHTNESS,
	CAPTURE_CONTRAST,
	CAPTURE_HUE,
	CAPTURE_SATURATION,
	CAPTURE_SHARPNESS,
	CAPTURE_GAMMA,
	CAPTURE_COLORENABLE,
	CAPTURE_WHITEBALANCE,
	CAPTURE_BACKLIGHTCOMPENSATION,
	CAPTURE_GAIN,
	CAPTURE_PAN,
	CAPTURE_TILT,
	CAPTURE_ROLL,
	CAPTURE_ZOOM,
	CAPTURE_EXPOSURE,
	CAPTURE_IRIS,
	CAPTURE_FOCUS,
	CAPTURE_PROP_MAX
} ;

typedef void (*hotplug_event_t)(void* context, size_t device, bool added);

/* Sets up the ESCAPI DLL and the function pointers below. Call this first! */
/* Returns number of capture devices found (same as countCaptureDevices, below) */
extern size_t setupESCAPI(hotplug_event_t fkt, void* context);

/* return the number of capture devices found */
typedef size_t (*countCaptureDevicesProc)(void);

typedef void (*setCaptureDeviceHotplugFunctionProc)(hotplug_event_t fkt, void* context);

/* Return the number of device IDS copied to the buffer, the number of devices if buffer is NULL */
typedef size_t (*getCaptureDeviceIdsProc)(size_t* buffer, size_t count);

typedef size_t (*getCaptureSupportedFormatsAndResolutionsProc)(size_t deviceno, SimpleFormat* formats, size_t* widths,
                                                     size_t* heights, size_t count);

/* initCapture tries to open the video capture device.
 * Returns 0 on failure, 1 on success.
 * Note: Capture parameter values must not change while capture device
 *       is in use (i.e. between initCapture and deinitCapture).
 *       Do *not* free the target buffer, or change its pointer!
 */
typedef int (*initCaptureProc)(size_t deviceno, struct SimpleCapParams* aParams);

/* deinitCapture closes the video capture device. */
typedef void (*deinitCaptureProc)(size_t deviceno);

/* doCapture requests video frame to be captured. */
typedef void (*doCaptureProc)(size_t deviceno);

/* isCaptureDone returns 1 when the requested frame has been captured.*/
typedef int (*isCaptureDoneProc)(size_t deviceno);

/* isCaptureDone returns 1 when the requested frame has been captured.*/
typedef size_t (*getCaptureImageProc)(size_t deviceno, char** buffer, size_t* stride, size_t* height);

/* Get the user-friendly name of a capture device. */
typedef size_t (*getCaptureDeviceNameProc)(size_t deviceno, char* namebuffer, size_t bufferlength);
typedef size_t (*getCaptureDeviceNameWProc)(size_t deviceno, wchar_t* namebuffer,
                                            size_t bufferlength);

/* Returns the ESCAPI DLL version. 0x200 for 2.0 */
typedef int (*ESCAPIVersionProc)(void);

/*
    On properties -
    - Not all cameras support properties at all.
    - Not all properties can be set to auto.
    - Not all cameras support all properties.
    - Messing around with camera properties may lead to weird results, so YMMV.
*/

typedef size_t (*getCapturePropertyListProc)(size_t deviceno, CAPTURE_PROPERTIES* prop, size_t count);

/* Gets value (0..1) of a camera property (see CAPTURE_PROPERTIES, above) */
typedef float (*getCapturePropertyValueProc)(size_t deviceno, CAPTURE_PROPERTIES prop);
typedef float (*getCapturePropertyMinProc)(size_t deviceno, CAPTURE_PROPERTIES prop);
typedef float (*getCapturePropertyMaxProc)(size_t deviceno, CAPTURE_PROPERTIES prop);

/* Gets whether the property is set to automatic (see CAPTURE_PROPERTIES, above) */
typedef int (*getCapturePropertyAutoProc)(size_t deviceno, CAPTURE_PROPERTIES prop);
/* Set camera property to a value (0..1) and whether it should be set to auto. */
typedef int (*setCapturePropertyProc)(size_t deviceno, CAPTURE_PROPERTIES prop, float value, int autoval);

/*
    All error situations in ESCAPI are considered catastrophic. If such should
    occur, the following functions can be used to check which line reported the
    error, and what the HRESULT of the error was. These may help figure out
    what the problem is.
*/

/* Return line number of error, or 0 if no catastrophic error has occurred. */
typedef int (*getCaptureErrorLineProc)(size_t deviceno);
/* Return HRESULT of the catastrophic error, or 0 if none. */
typedef int (*getCaptureErrorCodeProc)(size_t deviceno);

/* initCaptureWithOptions allows additional options to be given. Otherwise it's identical with
 * initCapture
 */
typedef int (*initCaptureWithOptionsProc)(size_t deviceno, struct SimpleCapParams* aParams,
                                          unsigned int aOptions);

// Options accepted by above:
// Return raw data instead of converted rgb. Using this option assumes you know what you're doing.
#define CAPTURE_OPTION_RAWDATA 1
// Mask to check for valid options - all options OR:ed together.
#define CAPTURE_OPTIONS_MASK (CAPTURE_OPTION_RAWDATA)

#ifndef ESCAPI_DEFINITIONS_ONLY
extern setCaptureDeviceHotplugFunctionProc setCaptureDeviceHotplugFunction;
extern countCaptureDevicesProc countCaptureDevices;
extern getCaptureDeviceIdsProc getCaptureDeviceIds;
extern getCaptureSupportedFormatsAndResolutionsProc getCaptureSupportedFormatsAndResolutions;
extern initCaptureProc initCapture;
extern deinitCaptureProc deinitCapture;
extern doCaptureProc doCapture;
extern isCaptureDoneProc isCaptureDone;
extern getCaptureImageProc getCaptureImage;
extern getCaptureDeviceNameProc getCaptureDeviceName;
extern getCapturePropertyListProc getCapturePropertyList;
extern getCaptureDeviceNameWProc getCaptureDeviceNameW;
extern ESCAPIVersionProc ESCAPIVersion;
extern getCapturePropertyValueProc getCapturePropertyValue;
extern getCapturePropertyMinProc getCapturePropertyMin;
extern getCapturePropertyMaxProc getCapturePropertyMax;
extern getCapturePropertyAutoProc getCapturePropertyAuto;
extern setCapturePropertyProc setCaptureProperty;
extern getCaptureErrorLineProc getCaptureErrorLine;
extern getCaptureErrorCodeProc getCaptureErrorCode;
extern initCaptureWithOptionsProc initCaptureWithOptions;
#endif
