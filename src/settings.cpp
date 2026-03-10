#include "phonefinder/settings.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include "phonefinder/hotkey_rules.hpp"
#include "phonefinder/util.hpp"

namespace phonefinder {
namespace fs = std::filesystem;

static fs::path settings_path() {
#ifdef _WIN32
  const char* appdata = std::getenv("APPDATA");
  fs::path base = appdata ? fs::path(appdata) : fs::current_path();
  fs::path dir = base / "phonefinder";
#else
  const char* home = std::getenv("HOME");
  fs::path base = home ? fs::path(home) : fs::current_path();
  fs::path dir = base / ".config" / "phonefinder";
#endif
  fs::create_directories(dir);
  return dir / "settings.json";
}

AppSettings default_settings(const std::string& source_path) {
  AppSettings s;
  s.source_path = source_path;
  s.trigger_hotkey = kDefaultHotkey;
  return s;
}

static std::string json_get(const std::string& text, const std::string& key) {
  std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch m;
  if (std::regex_search(text, m, re) && m.size() == 2) {
    return m[1].str();
  }
  return "";
}

AppSettings load_settings(const std::string& default_source) {
  AppSettings s = default_settings(default_source);
  const fs::path p = settings_path();
  if (!fs::exists(p)) {
    return s;
  }

  std::ifstream fin(p);
  if (!fin) {
    return s;
  }
  std::stringstream buf;
  buf << fin.rdbuf();
  const std::string text = buf.str();

  std::string source = json_get(text, "sourcePath");
  if (!source.empty()) {
    s.source_path = source;
  }

  std::string hotkey = json_get(text, "triggerHotkey");
  std::string normalized;
  std::string err;
  if (normalize_hotkey(hotkey, normalized, err)) {
    s.trigger_hotkey = normalized;
  }

  return s;
}

bool save_settings(const AppSettings& settings, std::string& err) {
  std::string normalized;
  if (!normalize_hotkey(settings.trigger_hotkey, normalized, err)) {
    return false;
  }

  const fs::path p = settings_path();
  std::ofstream out(p, std::ios::trunc);
  if (!out) {
    err = "open settings file failed";
    return false;
  }

  out << "{\n"
      << "  \"sourcePath\": \"" << settings.source_path << "\",\n"
      << "  \"triggerHotkey\": \"" << normalized << "\"\n"
      << "}\n";
  err.clear();
  return true;
}

}  // namespace phonefinder
