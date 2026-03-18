#pragma once

#include "managers/save/save_serializer.hpp"
#include "managers/save/save_result.hpp"
#include "models/new_game_model.hpp"
#include "entities/level.hpp"
#include "constants.hpp"

namespace TyraCraft {

/**
 * @brief Base class for save file migrations
 *
 * Each migration represents the transformation from one save version to another.
 * For example, MigrationV1ToV2 reads V1 format and populates V3 in-memory structures.
 *
 * This eliminates code duplication: instead of LoadSavedGameV1, LoadSavedGameV2, etc.,
 * we define only the *differences* between versions.
 *
 * Usage pattern:
 *   gzFile f = gzopen(path, "rb");
 *   SaveSerializer serializer(f);
 *   MigrationV1ToV2 migration;
 *
 *   NewGameOptions* options = nullptr;
 *   LevelMap* map = nullptr;
 *   SaveResult result = migration.Apply(serializer, options, map, ...);
 *
 *   gzclose(f);
 */
class SaveMigration {
 public:
  virtual ~SaveMigration() = default;

  /**
   * Apply migration from old format to current V1 format in memory
   * @param serializer Positioned at first data byte after magic+version
   * @param outOptions Pointer to NewGameOptions to populate
   * @param outMap Pointer to LevelMap to populate
   * @param outPlayerPos Player position to populate
   * @param outCameraPitch Camera pitch to populate
   * @param outCameraYaw Camera yaw to populate
   * @param outTicksCounter Game ticks to populate
   * @param outElapsedRealTime Elapsed real time to populate
   * @param outTicksDayCounter Day ticks to populate
   */
  virtual SaveResult Apply(SaveSerializer& serializer,
                          NewGameOptions* outOptions,
                          LevelMap* outMap,
                          Tyra::Vec4* outPlayerPos,
                          float* outCameraPitch,
                          float* outCameraYaw,
                          uint64_t* outTicksCounter,
                          uint64_t* outElapsedRealTime,
                          uint64_t* outTicksDayCounter) = 0;

  virtual int GetSourceVersion() const = 0;
};

// ============================================================================
// IDENTITY MIGRATION V1
// ============================================================================

/**
 * @brief Identity migration for the new folder-based format (V1)
 */
class MigrationV1 : public SaveMigration {
 public:
  SaveResult Apply(SaveSerializer& serializer,
                   NewGameOptions* outOptions,
                   LevelMap* outMap,
                   Tyra::Vec4* outPlayerPos,
                   float* outCameraPitch,
                   float* outCameraYaw,
                   uint64_t* outTicksCounter,
                   uint64_t* outElapsedRealTime,
                   uint64_t* outTicksDayCounter) override;

  int GetSourceVersion() const override { return 1; }
};

}  // namespace TyraCraft
