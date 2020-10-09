#pragma once

#include <vector>

typedef void (*IMAGE_TRANSFORM_FN)(std::vector<char>& aDest, size_t& aDestStride, const BYTE* aSrc, size_t aSrcStride,
								   size_t aWidthInPixels, size_t aHeightInPixels);

struct ConversionFunction
{
	GUID mSubtype;
	IMAGE_TRANSFORM_FN mXForm;
};

extern IMAGE_TRANSFORM_FN gFormatRawConverter;
extern std::vector<ConversionFunction> gFormatConversions;
