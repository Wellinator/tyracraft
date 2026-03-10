#pragma once

#include "managers/save/save_migration.hpp"
#include "managers/save/save_serializer.hpp"
#include "models/new_game_model.hpp"
#include "entities/level.hpp"
#include "tyra"

namespace TyraCraft {

/**
 * @brief Manages save file migrations
 *
 * Factory and coordinator for applying the correct migration based on save version.
 * Handles all supported versions (1, 2, 3) and provides clear error messages for unsupported ones.
 *
 * Usage:
 *   MigrationManager manager;
 *   SaveMigration* migration = manager.GetMigration(versionRead);
 *   if (!migration) {
 *     error("Unsupported save version");
 *   }
 *
 *   SaveResult result = migration->Apply(serializer, options, map, ...);
 */
class MigrationManager {
 public:
  static constexpr int CURRENT_VERSION = 3;
  static constexpr int MIN_SUPPORTED_VERSION = 1;
  static constexpr int MAX_SUPPORTED_VERSION = 3;

  MigrationManager();
  ~MigrationManager();

  /**
   * Get migration handler for specific source version
   * @param version Source save version (1, 2, or 3)
   * @return Pointer to migration object, or nullptr if version not supported
   *
   * Caller must check for nullptr before using the migration.
   */
  SaveMigration* GetMigration(int version);

  /**
   * Check if version is supported
   */
  bool IsVersionSupported(int version) const {
    return version >= MIN_SUPPORTED_VERSION && version <= MAX_SUPPORTED_VERSION;
  }

  /**
   * Get human-readable error message for unsupported version
   */
  static std::string GetVersionErrorMessage(int version);

 private:
  // Migration instances (owned by manager)
  MigrationV1ToV2 migrationV1;
  MigrationV2ToV3 migrationV2;
  MigrationV3 migrationV3;
};

}  // namespace TyraCraft
