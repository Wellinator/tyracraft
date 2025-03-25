#pragma once

#include "constants.hpp"
#include <tyra>
#include <vector>

using Tyra::Vec4;

class PathResult {
 public:
  PathResult() { waypoints.reserve(CHUNK_SIZE * 2); }

  size_t currentIndex = 0;
  std::vector<Vec4> waypoints = {};
};
