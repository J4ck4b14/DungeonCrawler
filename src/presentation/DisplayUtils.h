#pragma once

#include <algorithm>
#include <string>

namespace DisplayUtils {

inline std::string MakeMeter(int current, int maximum, int width = 16) {
	if (width < 1) width = 1;
	if (maximum < 1) maximum = 1;
	current = std::clamp(current, 0, maximum);
	const int filled = current * width / maximum;
	return "[" + std::string(filled, '#') + std::string(width - filled, '-') + "]";
}

} // namespace DisplayUtils
