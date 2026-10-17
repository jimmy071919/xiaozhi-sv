#ifndef MARKDOWN_TEXT_H
#define MARKDOWN_TEXT_H

#include <cstddef>
#include <string>
#include <string_view>

// Convert agent-style Markdown (bold, inline code, headings, lists, links,
// code fences) into plain text suitable for a small screen. Bullets become
// "• " and links keep only their label.
std::string StripMarkdownForDisplay(std::string_view text);

// Truncate to at most max_bytes without splitting a UTF-8 sequence.
std::string TruncateUtf8(std::string_view text, size_t max_bytes);

#endif  // MARKDOWN_TEXT_H
