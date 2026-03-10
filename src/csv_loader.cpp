#include "phonefinder/csv_loader.hpp"

#include <filesystem>
#include <fstream>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <chrono>

#include "phonefinder/pinyin.hpp"
#include "phonefinder/util.hpp"

namespace phonefinder {
namespace fs = std::filesystem;

static std::time_t file_time_to_time_t(const fs::file_time_type& ft) {
  const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ft - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
  return std::chrono::system_clock::to_time_t(sctp);
}

static std::vector<std::string> parse_csv_line(const std::string& line) {
  std::vector<std::string> out;
  std::string cur;
  bool in_quotes = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];
    if (ch == '"') {
      if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
        cur.push_back('"');
        ++i;
      } else {
        in_quotes = !in_quotes;
      }
      continue;
    }
    if (ch == ',' && !in_quotes) {
      out.push_back(cur);
      cur.clear();
      continue;
    }
    cur.push_back(ch);
  }
  out.push_back(cur);
  return out;
}

static std::string cell(const std::vector<std::string>& row, int idx) {
  if (idx < 0 || static_cast<std::size_t>(idx) >= row.size()) {
    return "";
  }
  return trim(row[static_cast<std::size_t>(idx)]);
}

static std::string normalize_header_key(std::string s) {
  s = trim(s);
  // Strip UTF-8 BOM bytes if present in cell.
  if (s.size() >= 3 &&
      static_cast<unsigned char>(s[0]) == 0xEF &&
      static_cast<unsigned char>(s[1]) == 0xBB &&
      static_cast<unsigned char>(s[2]) == 0xBF) {
    s = s.substr(3);
  }
  // Collapse ASCII spaces for robust matching.
  std::string out;
  out.reserve(s.size());
  for (unsigned char ch : s) {
    if (ch == ' ' || ch == '\t') continue;
    out.push_back(static_cast<char>(ch));
  }
  return upper_ascii(out);
}

static bool looks_like_phone(const std::string& s) {
  std::string t = trim(s);
  if (t.empty()) return false;
  int digits = 0;
  for (unsigned char ch : t) {
    if (std::isdigit(ch)) {
      ++digits;
      continue;
    }
    if (ch == '/' || ch == '-' || ch == '(' || ch == ')' || ch == '+' || ch == ' ') {
      continue;
    }
    return false;
  }
  return digits >= 3;
}

Dataset load_dataset_csv(const std::string& path) {
  const fs::path source_path = fs::u8path(path);
  if (!fs::exists(source_path)) {
    throw std::runtime_error("source file not found: " + path);
  }

  std::ifstream fin(source_path, std::ios::binary);
  if (!fin) {
    throw std::runtime_error("open file failed: " + path);
  }

  std::vector<std::vector<std::string>> rows;
  std::string line;
  bool first = true;
  while (std::getline(fin, line)) {
    if (first) {
      first = false;
      if (line.size() >= 3 &&
          static_cast<unsigned char>(line[0]) == 0xEF &&
          static_cast<unsigned char>(line[1]) == 0xBB &&
          static_cast<unsigned char>(line[2]) == 0xBF) {
        line = line.substr(3);
      }
    }
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    rows.push_back(parse_csv_line(line));
  }

  Dataset ds;
  ds.source_path = path;
  ds.mod_time = file_time_to_time_t(fs::last_write_time(source_path));

  if (rows.empty()) {
    return ds;
  }

  std::unordered_map<std::string, int> idx;
  auto header = rows.front();
  for (std::size_t i = 0; i < header.size(); ++i) {
    idx.emplace(normalize_header_key(header[i]), static_cast<int>(i));
  }

  int dept_idx = -1;
  int sub_idx = -1;
  int phone_idx = -1;
  int initials_idx = -1;
  int full_idx = -1;

  auto set_idx = [&](const std::string& key, int& target) {
    auto it = idx.find(key);
    if (it != idx.end()) target = it->second;
  };

  set_idx(normalize_header_key("科室"), dept_idx);
  set_idx(normalize_header_key("科室名称"), dept_idx);
  set_idx("DEPARTMENT", dept_idx);
  set_idx(normalize_header_key("细分"), sub_idx);
  set_idx(normalize_header_key("分组"), sub_idx);
  set_idx(normalize_header_key("岗位"), sub_idx);
  set_idx("SUBDIVISION", sub_idx);
  set_idx(normalize_header_key("电话"), phone_idx);
  set_idx(normalize_header_key("号码"), phone_idx);
  set_idx("PHONE", phone_idx);
  set_idx(normalize_header_key("首字母"), initials_idx);
  set_idx(normalize_header_key("拼音"), initials_idx);
  set_idx("INITIALS", initials_idx);
  set_idx(normalize_header_key("全拼"), full_idx);
  set_idx("FULLPINYIN", full_idx);

  std::size_t start = 1;
  if (dept_idx < 0 || phone_idx < 0) {
    dept_idx = 0;
    sub_idx = -1;
    initials_idx = -1;
    full_idx = -1;
    start = 0;

    // Auto-detect phone column for headerless/irregular files.
    int best_col = -1;
    int best_score = -1;
    for (int col = 1; col <= 3; ++col) {
      int score = 0;
      const std::size_t probe = std::min<std::size_t>(rows.size(), 24);
      for (std::size_t i = 0; i < probe; ++i) {
        if (looks_like_phone(cell(rows[i], col))) {
          ++score;
        }
      }
      if (score > best_score) {
        best_score = score;
        best_col = col;
      }
    }
    phone_idx = best_col >= 0 ? best_col : 1;
    if (phone_idx == 2) {
      sub_idx = 1;
    } else if (phone_idx == 1) {
      initials_idx = 2;
      full_idx = 3;
    }

    const std::string h0 = normalize_header_key(cell(rows.front(), 0));
    if (h0 == normalize_header_key("科室") || h0 == "DEPARTMENT") {
      start = 1;
    }
  }

  for (std::size_t i = start; i < rows.size(); ++i) {
    const auto& row = rows[i];
    std::string dept = cell(row, dept_idx);
    std::string sub = cell(row, sub_idx);
    std::string phone = cell(row, phone_idx);
    if (dept.empty() || phone.empty()) {
      continue;
    }
    if (!sub.empty()) {
      dept += "-" + sub;
    }
    Entry e;
    e.department = dept;
    e.phone = phone;
    e.initials = upper_ascii(cell(row, initials_idx));
    e.full_pinyin = upper_ascii(cell(row, full_idx));
    if (e.initials.empty()) {
      e.initials = build_initials(e.department);
    }
    if (e.full_pinyin.empty()) {
      e.full_pinyin = build_full_pinyin(e.department);
    }
    ds.entries.push_back(std::move(e));
  }

  return ds;
}

}  // namespace phonefinder
