#pragma once
#include <inttypes.h>

class NewGameOptions {
 public:
  NewGameOptions(){};
  ~NewGameOptions(){};

  uint32_t seed = 0;
  WorldType type = WorldType::WORLD_TYPE_ORIGINAL;
  float initialTime = 6000;
};
