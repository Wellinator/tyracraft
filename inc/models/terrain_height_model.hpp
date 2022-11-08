#pragma once
#include "contants.hpp"

class TerrainHeightModel {
 public:
  TerrainHeightModel(){};
  ~TerrainHeightModel(){};

  float minHeight = OVERWORLD_MIN_DISTANCE * DUBLE_BLOCK_SIZE;
  float maxHeight = OVERWORLD_MAX_DISTANCE * DUBLE_BLOCK_SIZE;

  void print() {
    TYRA_LOG("MIN -> ", minHeight);
    TYRA_LOG("MAX -> ", maxHeight);
    TYRA_LOG("");
  }
};
