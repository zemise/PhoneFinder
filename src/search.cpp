#include "phonefinder/search.hpp"

#include <algorithm>

#include "phonefinder/pinyin.hpp"
#include "phonefinder/util.hpp"

namespace phonefinder {

static int score_entry(const Entry& e, const std::string& q) {
  std::string initials = upper_ascii(trim(e.initials));
  if (initials.empty()) {
    initials = build_initials(e.department);
  }
  std::string full = upper_ascii(trim(e.full_pinyin));
  if (full.empty()) {
    full = build_full_pinyin(e.department);
  }
  const std::string name = upper_ascii(e.department);
  const std::string phone = upper_ascii(e.phone);

  int score = 0;

  if (!initials.empty()) {
    if (initials == q) score += 100;
    else if (starts_with(initials, q)) score += 80;
    else if (initials.find(q) != std::string::npos) score += 60;
  }

  if (!full.empty()) {
    if (full == q) score += 95;
    else if (starts_with(full, q)) score += 75;
    else if (full.find(q) != std::string::npos) score += 55;
  }

  if (name.find(q) != std::string::npos) score += 30;
  if (phone.find(q) != std::string::npos) score += 20;
  return score;
}

std::vector<MatchResult> search_entries(const std::vector<Entry>& entries,
                                        const std::string& query,
                                        int limit) {
  const std::string q = upper_ascii(trim(query));
  if (q.empty()) {
    return {};
  }

  std::vector<MatchResult> out;
  out.reserve(entries.size());
  for (const auto& e : entries) {
    int score = score_entry(e, q);
    if (score > 0) {
      out.push_back(MatchResult{e, score});
    }
  }

  std::stable_sort(out.begin(), out.end(), [](const MatchResult& a, const MatchResult& b) {
    if (a.score == b.score) {
      return a.entry.department < b.entry.department;
    }
    return a.score > b.score;
  });

  if (limit > 0 && static_cast<int>(out.size()) > limit) {
    out.resize(static_cast<std::size_t>(limit));
  }
  return out;
}

}  // namespace phonefinder
