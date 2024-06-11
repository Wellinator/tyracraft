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
  GameMode gameMode = GameMode::Creative;
  float initialTime = 1000;  // Sun rise in ticks
  std::string texturePack = "default";
  std::string name;
};
