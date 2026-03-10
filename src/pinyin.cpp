#include "phonefinder/pinyin.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "phonefinder/pinyin_map_generated.hpp"
#include "phonefinder/util.hpp"

namespace phonefinder {

namespace {

std::vector<char32_t> decode_utf8(const std::string& input) {
  std::vector<char32_t> out;
  out.reserve(input.size());
  for (std::size_t i = 0; i < input.size();) {
    const unsigned char c0 = static_cast<unsigned char>(input[i]);
    if (c0 < 0x80) {
      out.push_back(static_cast<char32_t>(c0));
      ++i;
      continue;
    }

    char32_t cp = 0;
    int more = 0;
    if ((c0 & 0xE0) == 0xC0) {
      cp = c0 & 0x1F;
      more = 1;
    } else if ((c0 & 0xF0) == 0xE0) {
      cp = c0 & 0x0F;
      more = 2;
    } else if ((c0 & 0xF8) == 0xF0) {
      cp = c0 & 0x07;
      more = 3;
    } else {
      ++i;
      continue;
    }

    if (i + static_cast<std::size_t>(more) >= input.size()) {
      break;
    }

    bool ok = true;
    for (int j = 1; j <= more; ++j) {
      const unsigned char cx = static_cast<unsigned char>(input[i + static_cast<std::size_t>(j)]);
      if ((cx & 0xC0) != 0x80) {
        ok = false;
        break;
      }
      cp = (cp << 6) | static_cast<char32_t>(cx & 0x3F);
    }
    if (ok) {
      out.push_back(cp);
      i += static_cast<std::size_t>(more + 1);
    } else {
      ++i;
    }
  }
  return out;
}

const PinyinToken* lookup_pinyin(char32_t cp) {
  const auto it = std::lower_bound(
      kPinyinMapData.begin(), kPinyinMapData.end(), cp,
      [](const PinyinMapEntry& entry, char32_t value) { return entry.cp < value; });
  if (it == kPinyinMapData.end() || it->cp != cp) {
    return nullptr;
  }
  return &it->token;
}

std::string build_pinyin_like(const std::string& text, bool initials_only) {
  std::string out;
  const auto cps = decode_utf8(text);
  for (char32_t cp : cps) {
    if (cp <= 0x7F) {
      // go-pinyin ignores ASCII letters/digits in mixed Chinese text.
      continue;
    }
    const PinyinToken* token = lookup_pinyin(cp);
    if (token == nullptr) {
      continue;
    }
    out += initials_only ? token->initials : token->full;
  }
  return upper_ascii(trim(out));
}

}  // namespace

std::string build_initials(const std::string& text) {
  return build_pinyin_like(text, true);
}

std::string build_full_pinyin(const std::string& text) {
  return build_pinyin_like(text, false);
}

}  // namespace phonefinder
