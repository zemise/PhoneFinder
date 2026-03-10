#include "phonefinder/service.hpp"

#include <chrono>
#include <filesystem>

#include "phonefinder/csv_loader.hpp"
#include "phonefinder/search.hpp"

namespace phonefinder {
namespace fs = std::filesystem;

static std::time_t file_time_to_time_t(const fs::file_time_type& ft) {
  const auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
      ft - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
  return std::chrono::system_clock::to_time_t(sctp);
}

Service::Service(std::string source_path) : source_(std::move(source_path)) {
  reload();
}

void Service::reload() {
  data_ = load_dataset_csv(source_);
}

bool Service::reload_if_changed() {
  const auto wt = fs::last_write_time(fs::u8path(source_));
  std::time_t t = file_time_to_time_t(wt);
  if (t <= data_.mod_time) {
    return false;
  }
  reload();
  return true;
}

std::vector<MatchResult> Service::search(const std::string& query, int limit) {
  reload_if_changed();
  return search_entries(data_.entries, query, limit);
}

std::size_t Service::count() const {
  return data_.entries.size();
}

}  // namespace phonefinder
