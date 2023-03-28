#pragma once
#include <inttypes.h>
#include <constants.hpp>

class NewGameOptions {
 public:
  NewGameOptions(){};
  ~NewGameOptions(){};

  uint32_t seed = 0;
  u8 drawDistance = MIN_DRAW_DISTANCE;
  WorldType type = WorldType::WORLD_TYPE_ORIGINAL;
  float initialTime = 6000;
  std::string texturePack = "default";
  std::string name;
};
