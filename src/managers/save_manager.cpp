#include "managers/save_manager.hpp"
#include "entities/level.hpp"
#include "utils.hpp"
#include "managers/save/save_serializer.hpp"
#include "managers/save/save_format.hpp"
#include "managers/save/migration_manager.hpp"

using namespace TyraCraft;

const int SaveManager::CurrentSaveVersion = 1;

static std::string getMetadataPath(const char* worldPath) {
  return std::string(worldPath) + "/data.tcw";
}

// ===========================================================================
// SAVE GAME
// ===========================================================================

SaveResult SaveManager::SaveGame(StateGamePlay* state, const char* fullPath) {
  if (!state || !fullPath) {
    return SaveResult::Failure("Invalid parameters (null state or path)");
  }

  // Ensure save directory exists
  if (!Utils::makeDirectoryRecursive(fullPath)) {
    return SaveResult::Failure("Failed to create save directory");
  }

  std::string metadataPath = getMetadataPath(fullPath);
  gzFile save_file = gzopen(metadataPath.c_str(), "wb");
  if (save_file == nullptr) {
    return SaveResult::Failure("Failed to open metadata file (.tcw) for writing");
  }

  SaveSerializer serializer(save_file);

  // Write header with magic number and version
  SaveResult result = SaveFormat::WriteHeader(serializer);
  if (!result) {
    gzclose(save_file);
    return result;
  }

  // Write world options
  result = SaveFormat::WriteWorldOptions(serializer, state->world->getWorldOptions());
  if (!result) {
    gzclose(save_file);
    return result;
  }

  // Write player state
  Vec4 playerPos = state->player->position;
  result = SaveFormat::WritePlayerState(serializer, playerPos,
                                        state->context->t_camera->pitch,
                                        state->context->t_camera->yaw);
  if (!result) {
    gzclose(save_file);
    return result;
  }

  // Write tick state
  result = SaveFormat::WriteTickState(serializer, g_ticksCounter,
                                      elapsedRealTime, ticksDayCounter);
  if (!result) {
    gzclose(save_file);
    return result;
  }

  // Flush and close file
  gzflush(save_file, Z_FINISH);
  gzclose(save_file);

  // Note: Level data (chunks) is saved separately by World/ChunkProvider
  state->plevel->saveAllChunks();
  state->world->getChunkProvider()->flush();

  TYRA_LOG("World metadata saved successfully to %s", metadataPath.c_str());
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

  std::string metadataPath = getMetadataPath(fullPath);
  gzFile save_file = gzopen(metadataPath.c_str(), "rb");
  if (save_file == nullptr) {
    return SaveResult::Failure("Failed to open metadata file (.tcw) inside world folder");
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

  // Simplified loading for folder-based system (Version 1+)
  NewGameOptions* gameOptions = state->world->getWorldOptions();
  Vec4 playerPos;
  float cameraPitch, cameraYaw;
  uint64_t loadedTicksCounter, loadedElapsedRealTime, loadedTicksDayCounter;

  // Read metadata using SaveFormat
  result = SaveFormat::ReadWorldOptions(serializer, gameOptions);
  if (result) result = SaveFormat::ReadPlayerState(serializer, &playerPos, &cameraPitch, &cameraYaw);
  if (result) result = SaveFormat::ReadTickState(serializer, &loadedTicksCounter, &loadedElapsedRealTime, &loadedTicksDayCounter);

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

  TYRA_LOG("World metadata loaded successfully from ", metadataPath.c_str());
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

  std::string metadataPath = getMetadataPath(fullPath);
  gzFile save_file = gzopen(metadataPath.c_str(), "rb");
  if (save_file == nullptr) {
    return model;
  }

  SaveSerializer serializer(save_file);

  // Read header
  int32_t version = 0;
  SaveResult result = SaveFormat::ReadHeader(serializer, version);
  if (!result) {
    gzclose(save_file);
    return model;
  }

  // V1 (New Folder Format): Use standard ReadWorldOptions
  result = SaveFormat::ReadWorldOptions(serializer, model);

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

  std::string metadataPath = getMetadataPath(fullPath);
  gzFile save_file = gzopen(metadataPath.c_str(), "rb");
  if (save_file == nullptr) {
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
    target->version = 0;
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
    gzclose(save_file);
    return;
  }

  target->version = version;

  // Read world name
  result = SaveFormat::ReadWorldNameOnly(serializer, &target->name);
  if (!result) {
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
  }

  gzclose(save_file);
}

bool SaveManager::CheckIfSaveExist(const char* fullPath) {
  struct stat buffer;
  if (stat(fullPath, &buffer) == 0 && S_ISDIR(buffer.st_mode)) {
    std::string metadataPath = getMetadataPath(fullPath);
    return (stat(metadataPath.c_str(), &buffer) == 0);
  }
  return false;
}

bool SaveManager::CheckIfFolderExist(const char* fullPath) {
  struct stat buffer;
  return (stat(fullPath, &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

static int remove_directory(const char* path) {
  std::vector<UtilDirectory> list = Utils::listDir(path);
  for (const auto& item : list) {
    std::string fullItemPath = std::string(path) + "/" + item.name;
    struct stat st;
    if (stat(fullItemPath.c_str(), &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        remove_directory(fullItemPath.c_str());
      } else {
        unlink(fullItemPath.c_str());
      }
    }
  }
  return rmdir(path);
}

int SaveManager::DeleteSave(const char* fullPath) {
  if (SaveManager::CheckIfFolderExist(fullPath)) {
    return remove_directory(fullPath);
  }
  return -1;
}

bool SaveManager::HasAvailableSaves() {
  std::vector<UtilDirectory> saveFilesList = Utils::listDir(FileUtils::fromCwd("saves/").c_str());
  
  for (size_t i = 0; i < saveFilesList.size(); i++) {
    const UtilDirectory dir = saveFilesList.at(i);
    if (dir.isDir) {
      std::string metadataPath = getMetadataPath(
          (std::string(FileUtils::fromCwd("saves/")) + dir.name).c_str());
      struct stat st;
      if (stat(metadataPath.c_str(), &st) == 0) {
        return true;
      }
    }
  }
  
  return false;
}
