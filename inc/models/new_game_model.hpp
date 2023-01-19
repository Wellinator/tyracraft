#pragma once
#include <inttypes.h>

class NewGameOptions {
 public:
  NewGameOptions(){};
  ~NewGameOptions(){};

  uint32_t seed = 0;
  u8 drawDistance = 2;
  WorldType type = WorldType::WORLD_TYPE_ORIGINAL;
  float initialTime = 6000;
  std::string texturePack = "default";
};
