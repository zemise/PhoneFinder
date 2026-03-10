#pragma once

#include <string>

namespace phonefinder {

constexpr const char* kDefaultHotkey = "F4*2";

bool normalize_hotkey(const std::string& input, std::string& normalized, std::string& err);

}  // namespace phonefinder
