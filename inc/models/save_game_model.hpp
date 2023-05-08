#pragma once
#include <inttypes.h>
#include <constants.hpp>
#include <tyra>
#include "new_game_model.hpp"
#include "entities/level.hpp"

using Tyra::Vec4;

class SaveGameModel {
 public:
  SaveGameModel(){};
  ~SaveGameModel(){};

  NewGameOptions gameOptions;
  Level worldLevel;
  Vec4 playerPosition;
  
};
