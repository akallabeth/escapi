/* "simplest", example of simply enumerating the available devices with ESCAPI */
#include <stdio.h>
#include "escapi.h"
#include <iostream>

static void hotplug(void* context, size_t device, bool added)
{
	std::cout << "device " << device << " was " << added << ", context " << context << std::endl;
}

int main()
{
	size_t i, j;

	/* Initialize ESCAPI */

	size_t devices = setupESCAPI(hotplug, (void*)0x12345);

	if (devices == 0)
	{
		printf("ESCAPI initialization failure or no devices found.\n");
		return -1;
	}

	size_t ids[64] = {};
	size_t idcount = getCaptureDeviceIds(ids, 64);
	if (idcount == 0)
	{
		printf("ESCAPI initialization failure or no devices found.\n");
		return -1;
	}

	char cname[64] = {};
	size_t clen = getCaptureDeviceName(ids[0], cname, 64);

	wchar_t wname[64] = {};
	size_t wlen = getCaptureDeviceNameW(ids[0], wname, 64);

	SimpleFormat formats[128] = {};
	size_t widths[128] = {};
	size_t heights[128] = {};
	size_t count = getCaptureSupportedFormatsAndResolutions(ids[0], formats, widths, heights, 128);
	if (count == 0)
	{
		printf("ESCAPI initialization failure, no resolutions detected for device.\n");
		return -1;
	}
	/* Set up capture parameters.
	 * ESCAPI will scale the data received from the camera
	 * (with point sampling) to whatever values you want.
	 * Typically the native resolution is 320*240.
	 */

	struct SimpleCapParams capture;
	capture.Format = formats[0];
	capture.mWidth = widths[0];
	capture.mHeight = heights[0];
	capture.mTargetBuf = new int[capture.mWidth * capture.mHeight];

	/* Initialize capture - only one capture may be active per device,
	 * but several devices may be captured at the same time.
	 *
	 * 0 is the first device.
	 */

	if (initCapture(ids[0], &capture) == 0)
	{
		printf("Capture failed - device may already be in use.\n");
		return -2;
	}

	CAPTURE_PROPETIES properties[64] = {};
	auto cnt = getCapturePropertyList(ids[0], properties, 64);
	for (size_t x=0; x<cnt; x++) {
		auto a = getCapturePropertyAuto(ids[0], properties[x]);
		auto b = getCapturePropertyValue(ids[x], properties[x]);
		auto c = getCapturePropertyMin(ids[x], properties[x]);
		auto d = getCapturePropertyMax(ids[x], properties[x]);
		std::cout << a << b << c << d << std::endl;
	}

	/* Go through 10 capture loops so that the camera has
	 * had time to adjust to the lighting conditions and
	 * should give us a sane image..
	 */
	for (i = 0; i < 10; i++)
	{
		/* request a capture */
		doCapture(ids[0]);

		while (isCaptureDone(ids[0]) == 0)
		{
			/* Wait until capture is done.
			 * Warning: if capture init failed, or if the capture
			 * simply fails (i.e, user unplugs the web camera), this
			 * will be an infinite loop.
			 */
		}
	}

	/* now we have the data.. what shall we do with it? let's
	 * render it in ASCII.. (using 3 top bits of green as the value)
	 */
	char light[] = " .,-o+O0@";
	for (i = 0; i < capture.mHeight; i++)
	{
		for (j = 0; j < capture.mWidth; j++)
		{
			printf("%c", light[(capture.mTargetBuf[i * 24 + j] >> 13) & 7]);
		}
		printf("\n");
	}

	deinitCapture(ids[0]);
	return 0;
}
