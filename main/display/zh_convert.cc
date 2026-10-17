#include "zh_convert.h"

#include <sdkconfig.h>

#if CONFIG_CONVERT_DISPLAY_TEXT_TO_TRADITIONAL

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

struct ZhCharPair {
    uint16_t simplified;
    uint16_t traditional;
};

struct ZhPhrase2 {
    std::array<uint16_t, 2> simplified;
    std::array<uint16_t, 2> traditional;
};

struct ZhPhrase3 {
    std::array<uint16_t, 3> simplified;
    std::array<uint16_t, 3> traditional;
};

struct ZhVocabulary {
    const char* traditional;
    const char* taiwan;
};

#include "zh_s2t_table.inc"

std::vector<char32_t> DecodeUtf8(std::string_view text) {
    std::vector<char32_t> out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        auto c = static_cast<unsigned char>(text[i]);
        int extra = c < 0x80           ? 0
                    : (c >> 5) == 0x6  ? 1
                    : (c >> 4) == 0xE  ? 2
                    : (c >> 3) == 0x1E ? 3
                                       : -1;
        if (extra < 0 || i + extra >= text.size()) {
            out.push_back(0xFFFD);
            i++;
            continue;
        }
        char32_t cp = extra == 0 ? c : c & (0x3F >> extra);
        for (int k = 1; k <= extra; k++) {
            cp = (cp << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
        }
        out.push_back(cp);
        i += extra + 1;
    }
    return out;
}

void AppendUtf8(char32_t cp, std::string& out) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

template <typename Phrase, size_t N, size_t Len>
const Phrase* FindPhrase(const Phrase (&table)[N], const std::array<uint16_t, Len>& key) {
    auto it = std::lower_bound(
        std::begin(table), std::end(table), key,
        [](const Phrase& p, const std::array<uint16_t, Len>& k) { return p.simplified < k; });
    return it != std::end(table) && it->simplified == key ? it : nullptr;
}

uint16_t ConvertChar(uint16_t cp) {
    auto it = std::lower_bound(std::begin(kZhS2TChars), std::end(kZhS2TChars), cp,
                               [](const ZhCharPair& p, uint16_t c) { return p.simplified < c; });
    return it != std::end(kZhS2TChars) && it->simplified == cp ? it->traditional : cp;
}

// Replace mainland vocabulary with Taiwan usage, e.g. 軟件 -> 軟體
std::string ApplyTaiwanVocabulary(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        std::string_view rest(text.data() + i, text.size() - i);
        const ZhVocabulary* match = nullptr;
        for (const auto& entry : kZhTwVocabulary) {
            if (rest.starts_with(entry.traditional)) {
                match = &entry;
                break;
            }
        }
        if (match != nullptr) {
            out += match->taiwan;
            i += strlen(match->traditional);
            continue;
        }
        // Copy one UTF-8 character
        size_t len = 1;
        while (i + len < text.size() &&
               (static_cast<unsigned char>(text[i + len]) & 0xC0) == 0x80) {
            len++;
        }
        out.append(text, i, len);
        i += len;
    }
    return out;
}

}  // namespace

std::string ConvertToTraditionalForDisplay(std::string_view text) {
    // Fast path: pure ASCII needs no conversion
    if (std::all_of(text.begin(), text.end(),
                    [](char c) { return (static_cast<unsigned char>(c) & 0x80) == 0; })) {
        return std::string(text);
    }

    auto cps = DecodeUtf8(text);
    auto is_bmp = [&](size_t i) { return cps[i] >= 0x80 && cps[i] < 0x10000; };
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < cps.size();) {
        if (i + 2 < cps.size() && is_bmp(i) && is_bmp(i + 1) && is_bmp(i + 2)) {
            std::array<uint16_t, 3> key = {static_cast<uint16_t>(cps[i]),
                                           static_cast<uint16_t>(cps[i + 1]),
                                           static_cast<uint16_t>(cps[i + 2])};
            if (auto phrase = FindPhrase(kZhS2TPhrases3, key)) {
                for (auto c : phrase->traditional) {
                    AppendUtf8(c, out);
                }
                i += 3;
                continue;
            }
        }
        if (i + 1 < cps.size() && is_bmp(i) && is_bmp(i + 1)) {
            std::array<uint16_t, 2> key = {static_cast<uint16_t>(cps[i]),
                                           static_cast<uint16_t>(cps[i + 1])};
            if (auto phrase = FindPhrase(kZhS2TPhrases2, key)) {
                for (auto c : phrase->traditional) {
                    AppendUtf8(c, out);
                }
                i += 2;
                continue;
            }
        }
        AppendUtf8(is_bmp(i) ? ConvertChar(static_cast<uint16_t>(cps[i])) : cps[i], out);
        i++;
    }
    return ApplyTaiwanVocabulary(out);
}

#else

std::string ConvertToTraditionalForDisplay(std::string_view text) { return std::string(text); }

#endif
