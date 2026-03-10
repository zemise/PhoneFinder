#pragma once

#include <string>
#include <vector>
#include <ctime>

namespace phonefinder {

struct Entry {
  std::string department;
  std::string phone;
  std::string initials;
  std::string full_pinyin;
};

struct MatchResult {
  Entry entry;
  int score = 0;
};

struct Dataset {
  std::string source_path;
  std::time_t mod_time = 0;
  std::vector<Entry> entries;
};

struct AppSettings {
  std::string source_path;
  std::string trigger_hotkey;
};

}  // namespace phonefinder
