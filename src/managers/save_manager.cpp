#include "managers/save_manager.hpp"
#include "entities/level.hpp"
#include "utils.hpp"
#include "managers/save/save_serializer.hpp"
#include "managers/save/save_format.hpp"
#include "managers/save/migration_manager.hpp"

using namespace TyraCraft;

const int SaveManager::CurrentSaveVersion = 3;

// ===========================================================================
// SAVE GAME
// ===========================================================================

SaveResult SaveManager::SaveGame(StateGamePlay* state, const char* fullPath) {
  if (!state || !fullPath) {
    return SaveResult::Failure("Invalid parameters (null state or path)");
  }

  // On PS2, atomic save with temp file + rename is problematic due to filesystem locking
  // Write directly to final location instead
  gzFile save_file = gzopen(fullPath, "wb");
  if (save_file == nullptr) {
    return SaveResult::Failure("Failed to open save file for writing (disk full or permission denied?)");
  }

  SaveSerializer serializer(save_file);

  // Write header with magic number and version
  SaveResult result = SaveFormat::WriteHeader(serializer);
  if (!result) {
    gzclose(save_file);
    TYRA_LOG("ERROR: WritHeader failed - ", result.errorMessage.c_str());
    return result;
  }

  // Write world options
  result = SaveFormat::WriteWorldOptions(serializer, state->world->getWorldOptions());
  if (!result) {
    gzclose(save_file);
    TYRA_LOG("ERROR: WriteWorldOptions failed - ", result.errorMessage.c_str());
    return result;
  }

  // Write player state
  Vec4 playerPos = state->player->position;
  result = SaveFormat::WritePlayerState(serializer, playerPos,
                                        state->context->t_camera->pitch,
                                        state->context->t_camera->yaw);
  if (!result) {
    gzclose(save_file);
    TYRA_LOG("ERROR: WritePlayerState failed - ", result.errorMessage.c_str());
    return result;
  }

  // Write tick state
  result = SaveFormat::WriteTickState(serializer, g_ticksCounter,
                                      elapsedRealTime, ticksDayCounter);
  if (!result) {
    gzclose(save_file);
    TYRA_LOG("ERROR: WriteTickState failed - ", result.errorMessage.c_str());
    return result;
  }

  // Write world structure
  LevelMap* t_map = &Level::getInstance()->map;
  result = SaveFormat::WriteWorldStructure(serializer, t_map);
  if (!result) {
    gzclose(save_file);
    TYRA_LOG("ERROR: WriteWorldStructure failed - ", result.errorMessage.c_str());
    return result;
  }

  // Write world data (blocks, light, metadata) — ~3MB
  result = SaveFormat::WriteWorldData(serializer, t_map);
  if (!result) {
    gzclose(save_file);
    TYRA_LOG("ERROR: WriteWorldData failed - ", result.errorMessage.c_str());
    return result;
  }

  // Flush and close file
  gzflush(save_file, Z_FINISH);
  
  int closeResult = gzclose(save_file);
  if (closeResult != Z_OK) {
    TYRA_LOG("ERROR: gzclose failed with code: ", closeResult);
    return SaveResult::Failure("Failed to close save file properly");
  }

  TYRA_LOG("Game saved successfully - Bytes: ", serializer.GetBytesWritten());

  return SaveResult::Success();
}

// ===========================================================================
// LOAD GAME (UNIFIED)
// ===========================================================================

SaveResult SaveManager::LoadSavedGame(StateGamePlay* state,
                                      const char* fullPath) {
  if (!state || !fullPath) {
    return SaveResult::Failure("Invalid parameters (null state or path)");
  }

  // Reset world before loading new data
  state->world->resetWorldData();

  // Open save file for reading
  gzFile save_file = gzopen(fullPath, "rb");
  if (save_file == nullptr) {
    return SaveResult::Failure("Failed to open save file");
  }

  SaveSerializer serializer(save_file);

  // Read and validate header
  int32_t version = 0;
  SaveResult result = SaveFormat::ReadHeader(serializer, version);
  if (!result) {
    gzclose(save_file);
    return result;
  }

  TYRA_LOG("Loading save version: ", version);

  // Get appropriate migration for this version
  MigrationManager migrationMgr;
  if (!migrationMgr.IsVersionSupported(version)) {
    gzclose(save_file);
    return SaveResult::Failure(
        MigrationManager::GetVersionErrorMessage(version));
  }

  SaveMigration* migration = migrationMgr.GetMigration(version);
  if (migration == nullptr) {
    gzclose(save_file);
    return SaveResult::Failure("Internal error: migration not found");
  }

  // Apply migration to load data in V3 format
  NewGameOptions* gameOptions = state->world->getWorldOptions();
  LevelMap* t_map = &state->plevel->map;
  Vec4 playerPos;
  float cameraPitch, cameraYaw;
  uint64_t loadedTicksCounter, loadedElapsedRealTime, loadedTicksDayCounter;

  result = migration->Apply(serializer, gameOptions, t_map, &playerPos,
                           &cameraPitch, &cameraYaw, &loadedTicksCounter,
                           &loadedElapsedRealTime, &loadedTicksDayCounter);

  if (!result) {
    gzclose(save_file);
    return result;
  }

  // Apply loaded state to game
  state->player->setPosition(playerPos);
  state->world->setSavedSpawnArea(playerPos);
  state->context->t_camera->pitch = cameraPitch;
  state->context->t_camera->yaw = cameraYaw;
  state->context->t_camera->targetPitch = cameraPitch;
  state->context->t_camera->targetYaw = cameraYaw;
  state->context->t_camera->smoothPitch = cameraPitch;
  state->context->t_camera->smoothYaw = cameraYaw;

  // Restore tick state
  g_ticksCounter = static_cast<uint32_t>(loadedTicksCounter);
  ::elapsedRealTime = static_cast<double>(loadedElapsedRealTime);
  ::ticksDayCounter = static_cast<u16>(loadedTicksDayCounter);

  // Close file
  gzclose(save_file);

  TYRA_LOG("Game loaded successfully - bytes read: ",
           serializer.GetBytesRead());

  return SaveResult::Success();
}

// ===========================================================================
// DEPRECATED: Legacy loaders (kept for backward compatibility)
// ===========================================================================

void SaveManager::LoadSavedGameV1(StateGamePlay* state,
                                  const gzFile& save_file) {
  // This function is deprecated. Use LoadSavedGame() which handles all versions.
  // Kept only for code that might call this directly.
  TYRA_LOG("WARNING: LoadSavedGameV1 is deprecated. Use LoadSavedGame() instead.");
}

void SaveManager::LoadSavedGameV2(StateGamePlay* state,
                                  const gzFile& save_file) {
  // This function is deprecated. Use LoadSavedGame() which handles all versions.
  TYRA_LOG("WARNING: LoadSavedGameV2 is deprecated. Use LoadSavedGame() instead.");
}

void SaveManager::LoadSavedGameV3(StateGamePlay* state,
                                  const gzFile& save_file) {
  // This function is deprecated. Use LoadSavedGame() which handles all versions.
  TYRA_LOG("WARNING: LoadSavedGameV3 is deprecated. Use LoadSavedGame() instead.");
}

// ===========================================================================
// GET WORLD OPTIONS FROM SAVE FILE
// ===========================================================================

NewGameOptions* SaveManager::GetNewGameOptionsFromSaveFile(
    const char* fullPath) {
  NewGameOptions* model = new NewGameOptions();

  gzFile save_file = gzopen(fullPath, "rb");
  if (save_file == nullptr) {
    TYRA_TRAP("Could not open save file at: ", fullPath);
    return model;  // Return default options if file can't be opened
  }

  SaveSerializer serializer(save_file);

  // Read header
  int32_t version = 0;
  SaveResult result = SaveFormat::ReadHeader(serializer, version);
  if (!result) {
    TYRA_LOG("Error reading save header: ", result.errorMessage.c_str());
    gzclose(save_file);
    return model;  // Return default options on error
  }

  // Check version support
  MigrationManager migrationMgr;
  if (!migrationMgr.IsVersionSupported(version)) {
    TYRA_LOG("Unsupported save version: ", version);
    gzclose(save_file);
    return model;  // Return default options for unsupported versions
  }

  // Read world options depending on version
  if (version <= 2) {
    // V1/V2: seed, gameMode, name, legacyDrawDistance, initialTime, type, texturePack
    result = serializer.ReadUInt32(&model->seed);
    if (!result) {
      gzclose(save_file);
      return model;
    }

    uint8_t gameMode;
    result = serializer.ReadUInt8(&gameMode);
    if (!result) {
      gzclose(save_file);
      return model;
    }
    model->gameMode = (GameMode)gameMode;

    result = serializer.ReadString(&model->name, SaveSerializer::MAX_SAVE_NAME_LENGTH);
    if (!result) {
      gzclose(save_file);
      return model;
    }

    // Legacy draw distance - map to enum
    u8 legacyDrawDistance;
    result = serializer.ReadUInt8(&legacyDrawDistance);
    if (!result) {
      gzclose(save_file);
      return model;
    }

    if (legacyDrawDistance <= 10)
      model->drawDistanceMode = DrawDistanceMode::Low;
    else if (legacyDrawDistance <= 16)
      model->drawDistanceMode = DrawDistanceMode::Medium;
    else if (legacyDrawDistance <= 24)
      model->drawDistanceMode = DrawDistanceMode::High;
    else
      model->drawDistanceMode = DrawDistanceMode::Auto;

    // Rest of options
    result = serializer.ReadFloat(&model->initialTime);
    if (!result) {
      gzclose(save_file);
      return model;
    }

    uint8_t worldType;
    result = serializer.ReadUInt8(&worldType);
    if (!result) {
      gzclose(save_file);
      return model;
    }
    model->type = (WorldType)worldType;

    result = serializer.ReadString(&model->texturePack,
                                   SaveSerializer::MAX_TEXTURE_PACK_NAME);
  } else {
    // V3: Use standard ReadWorldOptions
    result = SaveFormat::ReadWorldOptions(serializer, model);
  }

  if (!result) {
    TYRA_LOG("Error reading world options: ", result.errorMessage.c_str());
  }

  gzclose(save_file);
  return model;
}

// ===========================================================================
// SET SAVE INFO (metadata for UI)
// ===========================================================================

void SaveManager::SetSaveInfo(const char* fullPath, SaveInfoModel* target) {
  if (!target) return;

  gzFile save_file = gzopen(fullPath, "rb");
  if (save_file == nullptr) {
    // File doesn't exist or can't be opened - use defaults
    target->version = 0;
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
    return;
  }

  SaveSerializer serializer(save_file);

  // Read header
  int32_t version = 0;
  SaveResult result = SaveFormat::ReadHeader(serializer, version);
  if (!result) {
    // Invalid header - fallback to filename
    target->version = 0;
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
    gzclose(save_file);
    return;
  }

  target->version = version;

  // For unsupported versions, use filename as fallback
  MigrationManager migrationMgr;
  if (!migrationMgr.IsVersionSupported(version)) {
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
    gzclose(save_file);
    return;
  }

  // Read world name
  result = SaveFormat::ReadWorldNameOnly(serializer, &target->name);
  if (!result) {
    // On error reading name, fallback to filename
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
  }

  gzclose(save_file);
}

bool SaveManager::CheckIfSaveExist(const char* fullPath) {
  struct stat buffer;
  return (stat(fullPath, &buffer) == 0);
}

int SaveManager::DeleteSave(const char* fullPath) {
  if (SaveManager::CheckIfSaveExist(fullPath)) return unlink(fullPath);
  return -1;
}

bool SaveManager::HasAvailableSaves() {
  std::vector<UtilDirectory> saveFilesList = Utils::listDir(FileUtils::fromCwd("saves/").c_str());
  
  for (size_t i = 0; i < saveFilesList.size(); i++) {
    const UtilDirectory dir = saveFilesList.at(i);
    const std::string fileExtension = FileUtils::getExtensionOfFilename(dir.name);

    TYRA_LOG("Found save file: ", dir.name, " with extension: ", fileExtension);
    
    if (strncmp(fileExtension.c_str(), "tcw", 3) == 0) {
      return true;
    }
  }
  
  return false;
}
