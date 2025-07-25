#pragma once
#include "constants.hpp"

class TerrainHeightModel {
 public:
  float minHeight = OVERWORLD_MIN_HEIGH * DOUBLE_BLOCK_SIZE;
  float maxHeight = OVERWORLD_MAX_HEIGH * DOUBLE_BLOCK_SIZE;
  u8 lowerBlockType = static_cast<u8>(Blocks::VOID);
  u8 upperBlockType = static_cast<u8>(Blocks::VOID);

  void reset() {
    minHeight = OVERWORLD_MIN_HEIGH * DOUBLE_BLOCK_SIZE;
    maxHeight = OVERWORLD_MAX_HEIGH * DOUBLE_BLOCK_SIZE;
    lowerBlockType = static_cast<u8>(Blocks::VOID);
    upperBlockType = static_cast<u8>(Blocks::VOID);
  }

  void print() {
    TYRA_LOG("MIN -> ", minHeight);
    TYRA_LOG("MAX -> ", maxHeight);
    TYRA_LOG("");
  }
};
