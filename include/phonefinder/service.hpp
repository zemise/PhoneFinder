#pragma once

#include <vector>
#include "phonefinder/types.hpp"

namespace phonefinder {

class Service {
 public:
  explicit Service(std::string source_path);

  void reload();
  bool reload_if_changed();
  std::vector<MatchResult> search(const std::string& query, int limit);
  std::size_t count() const;

 private:
  std::string source_;
  Dataset data_;
};

}  // namespace phonefinder
