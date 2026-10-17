#include "markdown_text.h"

namespace {

std::string_view TrimLeft(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
        s.remove_prefix(1);
    }
    return s;
}

// "---", "***", "___" (optionally spaced) and table separators like "|---|:--|".
bool IsRuleOrTableSeparator(std::string_view line) {
    size_t marks = 0;
    for (char c : line) {
        if (c == '-' || c == '*' || c == '_') {
            marks++;
        } else if (c != ' ' && c != '\t' && c != '|' && c != ':') {
            return false;
        }
    }
    return marks >= 3;
}

// Strip block-level markers at the start of a line.
std::string_view StripLinePrefix(std::string_view line, std::string& out) {
    line = TrimLeft(line);
    while (!line.empty() && line.front() == '>') {
        line = TrimLeft(line.substr(1));
    }
    size_t hashes = 0;
    while (hashes < line.size() && line[hashes] == '#') {
        hashes++;
    }
    if (hashes > 0 && hashes <= 6 && (hashes == line.size() || line[hashes] == ' ')) {
        return TrimLeft(line.substr(hashes));
    }
    if (line.size() >= 2 && (line[0] == '-' || line[0] == '*' || line[0] == '+') &&
        line[1] == ' ') {
        out += "• ";
        return TrimLeft(line.substr(2));
    }
    return line;
}

void AppendInline(std::string_view line, std::string& out) {
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '*' || c == '`' || c == '~') {
            continue;
        }
        // "__bold__" markers; keep single underscores (snake_case names).
        if (c == '_' && i + 1 < line.size() && line[i + 1] == '_') {
            i++;
            continue;
        }
        // [label](url) -> label
        if (c == '[') {
            size_t close = line.find(']', i + 1);
            if (close != std::string_view::npos && close + 1 < line.size() &&
                line[close + 1] == '(') {
                size_t paren = line.find(')', close + 2);
                if (paren != std::string_view::npos) {
                    AppendInline(line.substr(i + 1, close - i - 1), out);
                    i = paren;
                    continue;
                }
            }
        }
        out += c;
    }
}

}  // namespace

std::string StripMarkdownForDisplay(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    while (!text.empty()) {
        size_t nl = text.find('\n');
        std::string_view line = text.substr(0, nl);
        text = nl == std::string_view::npos ? std::string_view() : text.substr(nl + 1);

        std::string_view trimmed = TrimLeft(line);
        // Drop code fences, table separators, and horizontal rules entirely.
        if (trimmed.starts_with("```") || IsRuleOrTableSeparator(trimmed)) {
            continue;
        }
        if (!out.empty() && out.back() != '\n') {
            out += '\n';
        }
        AppendInline(StripLinePrefix(line, out), out);
    }
    while (!out.empty() && (out.back() == '\n' || out.back() == ' ')) {
        out.pop_back();
    }
    return out;
}

std::string TruncateUtf8(std::string_view text, size_t max_bytes) {
    if (text.size() <= max_bytes) {
        return std::string(text);
    }
    size_t end = max_bytes;
    while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) {
        end--;
    }
    return std::string(text.substr(0, end));
}
