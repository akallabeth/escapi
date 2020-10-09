#include <initguid.h>
#include <cguid.h>
#include <mfapi.h>
#include "conversion.h"

static void TransformImage_RGB24(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
                          size_t aWidthInPixels, size_t aHeightInPixels);

static void TransformImage_RGB32(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
                          size_t aWidthInPixels, size_t aHeightInPixels);

static void TransformImage_YUY2(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
                         size_t aWidthInPixels, size_t aHeightInPixels);

static void TransformImage_UYVY(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
                                size_t aWidthInPixels, size_t aHeightInPixels);

static void TransformImage_NV12(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
                         size_t aWidthInPixels, size_t aHeightInPixels);

static void TransformImage_RAW(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
                                size_t aWidthInPixels, size_t aHeightInPixels);

IMAGE_TRANSFORM_FN gFormatRawConverter = TransformImage_RAW;

std::vector<ConversionFunction> gFormatConversions = { { MFVideoFormat_RGB32, TransformImage_RGB32 },
	                                        { MFVideoFormat_RGB24, TransformImage_RGB24 },
											{ MFVideoFormat_YUY2, TransformImage_YUY2 },
											{ MFVideoFormat_UYVY, TransformImage_UYVY },
	                                        { MFVideoFormat_NV12, TransformImage_NV12 } };

void TransformImage_RGB24(std::vector<char>& dst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
						  size_t aWidthInPixels, size_t aHeightInPixels)
{
	aDstStride = aWidthInPixels * 4;
	dst.resize(aDstStride * aHeightInPixels);

	auto aDst = reinterpret_cast<BYTE*>(dst.data());
	for (DWORD y = 0; y < aHeightInPixels; y++)
	{
		auto srcPel = reinterpret_cast<const RGBTRIPLE*>(aSrc);
		auto destPel = reinterpret_cast<DWORD*>(aDst);

		for (DWORD x = 0; x < aWidthInPixels; x++)
		{
			destPel[x] = ((srcPel[x].rgbtRed << 16) | (srcPel[x].rgbtGreen << 8) |
			              (srcPel[x].rgbtBlue << 0) | (0xff << 24));
		}

		aSrc += aSrcStride;
		aDst += aDstStride;
	}
}

void TransformImage_RGB32(std::vector<char>& aDst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
						  size_t aWidthInPixels, size_t aHeightInPixels)
{
	aDstStride = aWidthInPixels * 4;
	aDst.resize(aDstStride * aHeightInPixels);

	auto dst = reinterpret_cast<BYTE*>(aDst.data());
	MFCopyImage(dst, aDstStride, aSrc, aSrcStride, aWidthInPixels * 4, aHeightInPixels);
}

static BYTE Clip(int aClr)
{
	if (aClr < 0)
		return 0;
	else if (aClr > 255)
		return 255;
	else
		return aClr;
}

static RGBQUAD ConvertYCrCbToRGB(BYTE aY, BYTE aCr, BYTE aCb)
{
	RGBQUAD rgbq;

	auto c = static_cast<int>(aY) - 16;
	auto d = static_cast<int>(aCb) - 128;
	auto e = static_cast<int>(aCr) - 128;

	rgbq.rgbRed = Clip((298 * c + 409 * e + 128) >> 8);
	rgbq.rgbGreen = Clip((298 * c - 100 * d - 208 * e + 128) >> 8);
	rgbq.rgbBlue = Clip((298 * c + 516 * d + 128) >> 8);

	return rgbq;
}

void TransformImage_YUY2(std::vector<char>& dst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
						 size_t aWidthInPixels, size_t aHeightInPixels)
{
	aDstStride = aWidthInPixels * 4;
	dst.resize(aDstStride * aHeightInPixels);

	auto aDst = reinterpret_cast<BYTE*>(dst.data());
	for (DWORD y = 0; y < aHeightInPixels; y++)
	{
		auto destPel = reinterpret_cast<RGBQUAD*>(aDst);
		auto srcPel = reinterpret_cast<const WORD*>(aSrc);

		for (DWORD x = 0; x < aWidthInPixels; x += 2)
		{
			// Byte order is U0 Y0 V0 Y1

			auto y0 = LOBYTE(srcPel[x]);
			auto u0 = HIBYTE(srcPel[x]);
			auto y1 = LOBYTE(srcPel[x + 1]);
			auto v0 = HIBYTE(srcPel[x + 1]);

			destPel[x] = ConvertYCrCbToRGB(y0, v0, u0);
			destPel[x + 1] = ConvertYCrCbToRGB(y1, v0, u0);
		}

		aSrc += aSrcStride;
		aDst += aDstStride;
	}
}

void TransformImage_UYVY(std::vector<char>& dst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
						 size_t aWidthInPixels, size_t aHeightInPixels)
{
	aDstStride = aWidthInPixels * 4;
	dst.resize(aDstStride * aHeightInPixels);

	auto aDst = reinterpret_cast<BYTE*>(dst.data());
	for (DWORD y = 0; y < aHeightInPixels; y++)
	{
		auto destPel = reinterpret_cast<RGBQUAD*>(aDst);
		auto srcPel = reinterpret_cast<const WORD*>(aSrc);

		for (DWORD x = 0; x < aWidthInPixels; x += 2)
		{
			// Byte order is U0 Y0 V0 Y1

			auto y0 = HIBYTE(srcPel[x]);
			auto u0 = LOBYTE(srcPel[x]);
			auto y1 = HIBYTE(srcPel[x + 1]);
			auto v0 = LOBYTE(srcPel[x + 1]);

			destPel[x] = ConvertYCrCbToRGB(y0, v0, u0);
			destPel[x + 1] = ConvertYCrCbToRGB(y1, v0, u0);
		}

		aSrc += aSrcStride;
		aDst += aDstStride;
	}
}

void TransformImage_NV12(std::vector<char>& dst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
						 size_t aWidthInPixels, size_t aHeightInPixels)
{
	aDstStride = aWidthInPixels * 4;
	dst.resize(aDstStride * aHeightInPixels);

	auto aDst = reinterpret_cast<BYTE*>(dst.data());
	auto bitsY = aSrc;
	auto bitsCb = bitsY + (aHeightInPixels * aSrcStride);
	auto bitsCr = bitsCb + 1;

	for (UINT y = 0; y < aHeightInPixels; y += 2)
	{
		auto lineY1 = bitsY;
		auto lineY2 = bitsY + aSrcStride;
		auto lineCr = bitsCr;
		auto lineCb = bitsCb;

		auto dibLine1 = aDst;
		auto dibLine2 = aDst + aDstStride;

		for (UINT x = 0; x < aWidthInPixels; x += 2)
		{
			auto y0 = lineY1[0];
			auto y1 = lineY1[1];
			auto y2 = lineY2[0];
			auto y3 = lineY2[1];
			auto cb = lineCb[0];
			auto cr = lineCr[0];

			auto r = ConvertYCrCbToRGB(y0, cr, cb);
			dibLine1[0] = r.rgbBlue;
			dibLine1[1] = r.rgbGreen;
			dibLine1[2] = r.rgbRed;
			dibLine1[3] = 0; // Alpha

			r = ConvertYCrCbToRGB(y1, cr, cb);
			dibLine1[4] = r.rgbBlue;
			dibLine1[5] = r.rgbGreen;
			dibLine1[6] = r.rgbRed;
			dibLine1[7] = 0; // Alpha

			r = ConvertYCrCbToRGB(y2, cr, cb);
			dibLine2[0] = r.rgbBlue;
			dibLine2[1] = r.rgbGreen;
			dibLine2[2] = r.rgbRed;
			dibLine2[3] = 0; // Alpha

			r = ConvertYCrCbToRGB(y3, cr, cb);
			dibLine2[4] = r.rgbBlue;
			dibLine2[5] = r.rgbGreen;
			dibLine2[6] = r.rgbRed;
			dibLine2[7] = 0; // Alpha

			lineY1 += 2;
			lineY2 += 2;
			lineCr += 2;
			lineCb += 2;

			dibLine1 += 8;
			dibLine2 += 8;
		}

		aDst += (2 * aDstStride);
		bitsY += (2 * aSrcStride);
		bitsCr += aSrcStride;
		bitsCb += aSrcStride;
	}
}

void TransformImage_RAW(std::vector<char>& dst, size_t& aDstStride, const BYTE* aSrc, size_t aSrcStride,
						 size_t aWidthInPixels, size_t aHeightInPixels)
{
	aDstStride = aSrcStride;
	dst.resize(aDstStride * aHeightInPixels);
	memcpy(dst.data(), aSrc, dst.size());
}
