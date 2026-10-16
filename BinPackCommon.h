// Private input guards shared by the packer implementations.
#pragma once

#include "Rect.h"
#include <limits>

namespace rbp {
namespace detail {

inline bool IsValidBinSize(Int width, Int height)
{
	const Int max = std::numeric_limits<Int>::max();
	// The sum bound also keeps contact scores <= 2*(width+height) representable.
	return width > 0 && height > 0 && width <= max / height &&
		height <= max / 2 && width <= max / 2 - height;
}

inline bool FitsBin(Int width, Int height, Int binWidth, Int binHeight, bool allowFlip = true)
{
	return width > 0 && height > 0 &&
		((width <= binWidth && height <= binHeight) ||
		(allowFlip && height <= binWidth && width <= binHeight));
}

inline bool HasValidDimensions(const std::vector<RectSize> &rects)
{
	for (size_t i = 0; i < rects.size(); ++i)
		if (rects[i].width <= 0 || rects[i].height <= 0)
			return false;
	return true;
}

} // namespace detail
} // namespace rbp
