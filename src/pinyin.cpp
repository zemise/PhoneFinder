#include "phonefinder/pinyin.hpp"

#include <cctype>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "phonefinder/util.hpp"

namespace phonefinder {

namespace {

struct PinyinToken {
  const char* initials;
  const char* full;
};

const std::unordered_map<char32_t, PinyinToken> kPinyinMap = {
    {U'一', {"Y", "YI"}},       {U'三', {"S", "SAN"}},     {U'业', {"Y", "YE"}},
    {U'中', {"Z", "ZHONG"}},    {U'临', {"L", "LIN"}},     {U'事', {"S", "SHI"}},
    {U'二', {"E", "ER"}},       {U'产', {"C", "CHAN"}},    {U'人', {"R", "REN"}},
    {U'介', {"J", "JIE"}},      {U'会', {"H", "HUI"}},     {U'传', {"C", "CHUAN"}},
    {U'住', {"Z", "ZHU"}},      {U'保', {"B", "BAO"}},     {U'信', {"X", "XIN"}},
    {U'修', {"X", "XIU"}},      {U'值', {"Z", "ZHI"}},     {U'健', {"J", "JIAN"}},
    {U'儿', {"E", "ER"}},       {U'免', {"M", "MIAN"}},    {U'入', {"R", "RU"}},
    {U'全', {"Q", "QUAN"}},     {U'内', {"N", "NEI"}},     {U'分', {"F", "FEN"}},
    {U'办', {"B", "BAN"}},      {U'务', {"W", "WU"}},      {U'勤', {"Q", "QIN"}},
    {U'化', {"H", "HUA"}},      {U'区', {"Q", "QU"}},      {U'医', {"Y", "YI"}},
    {U'卫', {"W", "WEI"}},      {U'发', {"F", "FA"}},      {U'取', {"Q", "QU"}},
    {U'口', {"K", "KOU"}},      {U'台', {"T", "TAI"}},     {U'号', {"H", "HAO"}},
    {U'后', {"H", "HOU"}},      {U'向', {"X", "XIANG"}},   {U'吸', {"X", "XI"}},
    {U'呼', {"H", "HU"}},       {U'喉', {"H", "HOU"}},     {U'型', {"X", "XING"}},
    {U'域', {"Y", "YU"}},       {U'士', {"S", "SHI"}},     {U'备', {"B", "BEI"}},
    {U'复', {"F", "FU"}},       {U'外', {"W", "WAI"}},     {U'妇', {"F", "FU"}},
    {U'姨', {"Y", "YI"}},       {U'学', {"X", "XUE"}},     {U'实', {"S", "SHI"}},
    {U'宣', {"X", "XUAN"}},     {U'室', {"S", "SHI"}},     {U'导', {"D", "DAO"}},
    {U'射', {"S", "SHE"}},      {U'尿', {"N", "NIAO"}},    {U'工', {"G", "GONG"}},
    {U'床', {"C", "CHUANG"}},   {U'库', {"K", "KU"}},      {U'康', {"K", "KANG"}},
    {U'廊', {"L", "LANG"}},     {U'式', {"S", "SHI"}},     {U'微', {"W", "WEI"}},
    {U'心', {"X", "XIN"}},      {U'急', {"J", "JI"}},      {U'总', {"Z", "ZONG"}},
    {U'息', {"X", "XI"}},       {U'感', {"G", "GAN"}},     {U'房', {"F", "FANG"}},
    {U'手', {"S", "SHOU"}},     {U'技', {"J", "JI"}},      {U'抢', {"Q", "QIANG"}},
    {U'护', {"H", "HU"}},       {U'持', {"C", "CHI"}},     {U'挂', {"G", "GUA"}},
    {U'控', {"K", "KONG"}},     {U'支', {"Z", "ZHI"}},     {U'收', {"S", "SHOU"}},
    {U'救', {"J", "JIU"}},      {U'教', {"J", "JIAO"}},    {U'新', {"X", "XIN"}},
    {U'普', {"P", "PU"}},       {U'本', {"B", "BEN"}},     {U'术', {"S", "SHU"}},
    {U'杨', {"Y", "YANG"}},     {U'标', {"B", "BIAO"}},    {U'核', {"H", "HE"}},
    {U'检', {"J", "JIAN"}},     {U'楼', {"L", "LOU"}},     {U'殊', {"S", "SHU"}},
    {U'泌', {"M", "MI"}},       {U'注', {"Z", "ZHU"}},     {U'消', {"X", "XIAO"}},
    {U'液', {"Y", "YE"}},       {U'热', {"R", "RE"}},      {U'物', {"W", "WU"}},
    {U'特', {"T", "TE"}},       {U'班', {"B", "BAN"}},     {U'生', {"S", "SHENG"}},
    {U'电', {"D", "DIAN"}},     {U'疫', {"Y", "YI"}},      {U'疾', {"J", "JI"}},
    {U'病', {"B", "BING"}},     {U'瘤', {"L", "LIU"}},     {U'皮', {"P", "PI"}},
    {U'眼', {"Y", "YAN"}},      {U'神', {"S", "SHEN"}},    {U'科', {"K", "KE"}},
    {U'站', {"Z", "ZHAN"}},     {U'管', {"G", "GUAN"}},    {U'线', {"X", "XIAN"}},
    {U'经', {"J", "JING"}},     {U'维', {"W", "WEI"}},     {U'耳', {"E", "ER"}},
    {U'肝', {"G", "GAN"}},      {U'肠', {"C", "CHANG"}},   {U'肤', {"F", "FU"}},
    {U'肾', {"S", "SHEN"}},     {U'肿', {"Z", "ZHONG"}},   {U'腔', {"Q", "QIANG"}},
    {U'药', {"Y", "YAO"}},      {U'血', {"X", "XUE"}},     {U'西', {"X", "XI"}},
    {U'设', {"S", "SHE"}},      {U'诊', {"Z", "ZHEN"}},    {U'话', {"H", "HUA"}},
    {U'费', {"F", "FEI"}},      {U'走', {"Z", "ZOU"}},     {U'送', {"S", "SONG"}},
    {U'透', {"T", "TOU"}},      {U'道', {"D", "DAO"}},     {U'配', {"P", "PEI"}},
    {U'金', {"J", "JIN"}},      {U'门', {"M", "MEN"}},     {U'防', {"F", "FANG"}},
    {U'阿', {"A", "A"}},        {U'院', {"Y", "YUAN"}},    {U'预', {"Y", "YU"}},
    {U'验', {"Y", "YAN"}},      {U'骨', {"G", "GU"}},      {U'鼻', {"B", "BI"}},
};

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

std::string build_pinyin_like(const std::string& text, bool initials_only) {
  std::string out;
  const auto cps = decode_utf8(text);
  for (char32_t cp : cps) {
    if (cp <= 0x7F) {
      // go-pinyin ignores ASCII letters/digits in mixed Chinese text.
      continue;
    }
    const auto it = kPinyinMap.find(cp);
    if (it == kPinyinMap.end()) {
      continue;
    }
    out += initials_only ? it->second.initials : it->second.full;
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
