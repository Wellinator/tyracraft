#pragma once
#include "constants.hpp"

class TerrainHeightModel {
 public:
  float minHeight = OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE;
  float maxHeight = OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE;

  void print() {
    TYRA_LOG("MIN -> ", minHeight);
    TYRA_LOG("MAX -> ", maxHeight);
    TYRA_LOG("");
  }
};
