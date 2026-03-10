#include "phonefinder/hotkey_rules.hpp"

#include <set>
#include <sstream>
#include <vector>

#include "phonefinder/util.hpp"

namespace phonefinder {

static bool is_modifier(const std::string& token) {
  return token == "CTRL" || token == "SHIFT";
}

static bool is_fkey(const std::string& token) {
  static const std::set<std::string> k = {
      "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"};
  return k.find(token) != k.end();
}

static bool allow_shape(const std::vector<std::string>& parts,
                        const std::string& main_base,
                        int tap_count) {
  int total = static_cast<int>(parts.size()) + (tap_count == 2 ? 1 : 0);
  if (total < 2 || total > 3) {
    return false;
  }

  if (total == 2) {
    if (parts.size() == 2 && tap_count == 1) {
      return is_modifier(parts[0]) && !is_modifier(main_base);
    }
    if (parts.size() == 1 && tap_count == 2) {
      return is_fkey(main_base);
    }
    return false;
  }

  if (main_base == "ENTER" || main_base == "RETURN") {
    return false;
  }

  if (parts.size() == 3 && tap_count == 1) {
    return is_modifier(parts[0]) && is_modifier(parts[1]) && !is_modifier(main_base);
  }
  if (parts.size() == 2 && tap_count == 2) {
    return is_modifier(parts[0]) && !is_modifier(main_base);
  }
  return false;
}

bool normalize_hotkey(const std::string& input, std::string& normalized, std::string& err) {
  std::string h = upper_ascii(trim(input));
  if (h.empty()) {
    normalized = kDefaultHotkey;
    err.clear();
    return true;
  }

  std::vector<std::string> parts;
  std::stringstream ss(h);
  std::string item;
  std::set<std::string> seen;
  while (std::getline(ss, item, '+')) {
    item = trim(item);
    if (item.empty()) {
      err = "invalid hotkey";
      return false;
    }
    if (seen.count(item)) {
      err = "invalid hotkey";
      return false;
    }
    seen.insert(item);
    parts.push_back(item);
  }
  if (parts.empty()) {
    err = "invalid hotkey";
    return false;
  }

  std::string main = parts.back();
  std::string main_base = main;
  int tap = 1;
  if (main.find('*') != std::string::npos) {
    if (main.size() < 2 || main.substr(main.size() - 2) != "*2") {
      err = "only *2 is supported";
      return false;
    }
    main_base = main.substr(0, main.size() - 2);
    tap = 2;
  }
  if (main_base.empty()) {
    err = "invalid hotkey";
    return false;
  }
  if (parts.size() > 1 && is_modifier(main_base)) {
    err = "modifier key cannot be main key in combination";
    return false;
  }
  if (!allow_shape(parts, main_base, tap)) {
    err = "hotkey not allowed by rule";
    return false;
  }

  normalized.clear();
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) normalized += "+";
    normalized += parts[i];
  }
  err.clear();
  return true;
}

}  // namespace phonefinder
