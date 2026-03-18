#include "managers/save/migration_manager.hpp"

namespace TyraCraft {

MigrationManager::MigrationManager()
    : migrationV1() {
}

MigrationManager::~MigrationManager() {
}

SaveMigration* MigrationManager::GetMigration(int version) {
  if (version == 1) {
    return &migrationV1;
  }
  return nullptr;
}

std::string MigrationManager::GetVersionErrorMessage(int version) {
  if (version < MIN_SUPPORTED_VERSION) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "Save file format version %d is too old (minimum supported: %d)",
             version, MIN_SUPPORTED_VERSION);
    return std::string(buffer);
  } else if (version > MAX_SUPPORTED_VERSION) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "Save file format version %d is newer than this build (current: %d)",
             version, MAX_SUPPORTED_VERSION);
    return std::string(buffer);
  }
  return "Unknown version error";
}

}  // namespace TyraCraft
