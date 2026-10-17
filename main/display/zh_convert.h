#ifndef ZH_CONVERT_H
#define ZH_CONVERT_H

#include <string>
#include <string_view>

// Convert Simplified Chinese to Traditional Chinese (Taiwan) for display when
// CONFIG_CONVERT_DISPLAY_TEXT_TO_TRADITIONAL is enabled; otherwise returns the
// text unchanged. Uses OpenCC character and common phrase tables.
std::string ConvertToTraditionalForDisplay(std::string_view text);

#endif  // ZH_CONVERT_H
