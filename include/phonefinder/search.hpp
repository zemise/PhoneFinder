#pragma once

#include <vector>
#include "phonefinder/types.hpp"

namespace phonefinder {

std::vector<MatchResult> search_entries(const std::vector<Entry>& entries,
                                        const std::string& query,
                                        int limit);

}  // namespace phonefinder
