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
   * Apply migration from old format to current V3 format in memory
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
// MIGRATION V1 → V3
// ============================================================================

/**
 * @brief Migrate from save format V1 to V3
 *
 * Differences from V3:
 * - Player position uses `sizeof(Vec4)` instead of `sizeof(float)*4`
 * - Draw distance is raw u8 value, not DrawDistanceMode enum
 *   - Must map: ≤10 → Low, ≤16 → Medium, ≤24 → High, else → Auto
 *
 * This encodes the V1-specific quirks in one place.
 */
class MigrationV1ToV2 : public SaveMigration {
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

 private:
  static DrawDistanceMode MapLegacyDrawDistance(u8 legacyDistance);
};

// ============================================================================
// MIGRATION V2 → V3
// ============================================================================

/**
 * @brief Migrate from save format V2 to V3
 *
 * Differences from V3:
 * - Draw distance is raw u8 value, not DrawDistanceMode enum (same as V1)
 *
 * Most logic identical to V1 except draw distance mapping is the same.
 */
class MigrationV2ToV3 : public SaveMigration {
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

  int GetSourceVersion() const override { return 2; }

 private:
  static DrawDistanceMode MapLegacyDrawDistance(u8 legacyDistance);
};

// ============================================================================
// IDENTITY MIGRATION V3 → V3
// ============================================================================

/**
 * @brief Identity migration - read V3 directly without transformation
 *
 * Allows loading V3 saves through the migration system without special-casing.
 */
class MigrationV3 : public SaveMigration {
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

  int GetSourceVersion() const override { return 3; }
};

}  // namespace TyraCraft
