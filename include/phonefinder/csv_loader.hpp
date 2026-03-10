#pragma once

#include <string>
#include "phonefinder/types.hpp"

namespace phonefinder {

Dataset load_dataset_csv(const std::string& path);

}  // namespace phonefinder
