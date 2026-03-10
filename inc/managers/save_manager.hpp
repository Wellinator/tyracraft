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
#include "managers/save/save_result.hpp"

using Tyra::FileUtils;
using Tyra::Vec4;
using TyraCraft::SaveResult;

class SaveManager {
 public:
  /**
   * Save current game state to file with validation and error reporting
   * @param state Pointer to current game play state
   * @param fullPath Full path to save file (e.g., "saves/MyWorld.tcw")
   * @return SaveResult with success status and error message if failed
   *
   * Features:
   * - Saves to temporary file first, then renames on success
   * - Validates all writes succeed
   * - Returns error if disk full, path invalid, etc.
   * - Never corrupts existing save on failure
   */
  static SaveResult SaveGame(StateGamePlay* state, const char* fullPath);

  /**
   * Load saved game from file with automatic migration from V1/V2 to V3
   * @param state Pointer to game play state to populate
   * @param fullPath Full path to save file
   * @return SaveResult with success status and error message if failed
   *
   * Features:
   * - Detects save file version
   * - Applies migrations automatically (V1→V3, V2→V3)
   * - Validates file integrity (magic number, bounds checking)
   * - Returns clear error messages on corruption
   */
  static SaveResult LoadSavedGame(StateGamePlay* state, const char* fullPath);

  // DEPRECATED: These are now internal. Used only by migration system.
  // Public API should use LoadSavedGame() which handles all versions.
  static void LoadSavedGameV1(StateGamePlay* state, const gzFile& save_file);
  static void LoadSavedGameV2(StateGamePlay* state, const gzFile& save_file);
  static void LoadSavedGameV3(StateGamePlay* state, const gzFile& save_file);

  /**
   * Extract world options from save file without loading full world data
   * Useful for menu previews (world name, seed, game mode)
   * @param fullPath Full path to save file
   * @return Pointer to NewGameOptions populated from file, or nullptr on error
   *
   * Note: Caller must delete the returned pointer
   */
  static NewGameOptions* GetNewGameOptionsFromSaveFile(const char* fullPath);

  /**
   * Fill SaveInfoModel with metadata from save file  
   * @param fullPath Full path to save file
   * @param target Pointer to SaveInfoModel to populate (version, name)
   */
  static void SetSaveInfo(const char* fullPath, SaveInfoModel* target);

  /**
   * Check if save file exists
   */
  static bool CheckIfSaveExist(const char* fullPath);

  /**
   * Delete save file
   * @return 0 on success, -1 if file doesn't exist
   */
  static int DeleteSave(const char* fullPath);

  /**
   * Check if any save files exist in saves/ directory
   */
  static bool HasAvailableSaves();

  static const int CurrentSaveVersion;
};