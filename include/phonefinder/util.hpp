#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>

namespace phonefinder {

inline std::string trim(const std::string& in) {
  std::size_t b = 0;
  while (b < in.size() && std::isspace(static_cast<unsigned char>(in[b]))) {
    ++b;
  }
  std::size_t e = in.size();
  while (e > b && std::isspace(static_cast<unsigned char>(in[e - 1]))) {
    --e;
  }
  return in.substr(b, e - b);
}

inline std::string upper_ascii(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return s;
}

inline bool starts_with(const std::string& s, const std::string& prefix) {
  return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

inline std::vector<std::string> split_trimmed(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string token;
  while (std::getline(ss, token, delim)) {
    out.push_back(trim(token));
  }
  return out;
}

inline bool parse_alnum_hotkey_token(const std::string& token, int& key) {
  if (token.size() != 1) {
    return false;
  }
  const unsigned char ch = static_cast<unsigned char>(token[0]);
  if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
    key = static_cast<int>(ch);
    return true;
  }
  return false;
}

inline bool parse_function_hotkey_token(const std::string& token, int min_fn, int max_fn, int& fn) {
  if (token.size() < 2 || token[0] != 'F') {
    return false;
  }
  const int n = std::atoi(token.c_str() + 1);
  if (n < min_fn || n > max_fn) {
    return false;
  }
  fn = n;
  return true;
}

}  // namespace phonefinder
