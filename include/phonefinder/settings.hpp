#pragma once

#include <string>
#include "phonefinder/types.hpp"

namespace phonefinder {

AppSettings default_settings(const std::string& source_path);
AppSettings load_settings(const std::string& default_source);
bool save_settings(const AppSettings& settings, std::string& err);

}  // namespace phonefinder
