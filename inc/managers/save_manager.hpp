#pragma once
#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"
#include <string>
#include "managers/tick_manager.hpp"
#include "states/game_play/state_game_play.hpp"
#include "models/new_game_model.hpp"
#include "entities/World.hpp"
#include "entities/level.hpp"
#include <3libs/nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <zlib.h>

using Tyra::FileUtils;
using Tyra::Vec4;

class SaveManager {
 public:
  static void SaveGame(StateGamePlay* state, const char* fullPath);

  static void LoadSavedGame(StateGamePlay* state, const char* fullPath);

  static NewGameOptions* GetNewGameOptionsFromSaveFile(const char* fullPath);

  static bool CheckIfSaveExist(const char* fullPath);
};