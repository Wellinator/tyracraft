#pragma once

#include "entities/level.hpp"
#include "constants.hpp"
#include <tyra>
#include <math.h>
#include <array>

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Vec4;

class LightManager {
 public:
  LightManager(LevelMap* t_terrain);
  ~LightManager();

  void init();
  void update();

  static std::array<u8, 4> getCornersAOValues(
      std::array<u8, 8> blocksNeightbors);
  static u8 getVertexAO(bool side1, bool corner, bool side2);
  static float calcAOIntensity(u8 AOValue);

 private:
  LevelMap* terrain = nullptr;

  std::array<u8, (OVERWORLD_H_DISTANCE * OVERWORLD_H_DISTANCE)> shadowMap;
};
