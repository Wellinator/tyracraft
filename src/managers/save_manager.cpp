#include "managers/save_manager.hpp"
#include "entities/level.hpp"

const int SaveManager::CurrentSaveVersion = 3;

static DrawDistanceMode mapLegacyDrawDistance(u8 legacyDistance) {
  if (legacyDistance <= DRAW_DISTANCE_LOW_CAP)
    return DrawDistanceMode::Low;
  else if (legacyDistance <= DRAW_DISTANCE_MEDIUM_CAP)
    return DrawDistanceMode::Medium;
  else if (legacyDistance <= DRAW_DISTANCE_HIGH_CAP)
    return DrawDistanceMode::High;
  else
    return DrawDistanceMode::Auto;
}

void SaveManager::SaveGame(StateGamePlay* state, const char* fullPath) {
  gzFile save_file = gzopen(fullPath, "wb");

  if (save_file != nullptr) {
    // Save version
    gzwrite(save_file, &SaveManager::CurrentSaveVersion, sizeof(int));

    // World seed
    gzwrite(save_file, &state->world->getWorldOptions()->seed,
            sizeof(uint32_t));

    // Game mode
    gzwrite(save_file, &state->world->getWorldOptions()->gameMode,
            sizeof(uint8_t));

    // World name
    uint16_t worldNameSize =
        state->world->getWorldOptions()->name.size() * sizeof(char);
    gzwrite(save_file, &worldNameSize, sizeof(worldNameSize));
    gzwrite(save_file, state->world->getWorldOptions()->name.data(),
            worldNameSize);

    // World draw distance mode
    gzwrite(save_file, &state->world->getWorldOptions()->drawDistanceMode,
            sizeof(u8));

    // World initial time
    gzwrite(save_file, &state->world->getWorldOptions()->initialTime,
            sizeof(float));

    // World type
    gzwrite(save_file, &state->world->getWorldOptions()->type, sizeof(uint8_t));

    // Texture Pack
    uint16_t texturePackSize =
        state->world->getWorldOptions()->texturePack.size() * sizeof(char);
    gzwrite(save_file, &texturePackSize, sizeof(texturePackSize));
    gzwrite(save_file, state->world->getWorldOptions()->texturePack.data(),
            texturePackSize);

    // Player position
    Vec4 playerPos = state->player->position;
    gzwrite(save_file, &playerPos.xyzw, sizeof(float) * 4);

    // TODO: add hot inventory state to save file;

    // Camera direction
    gzwrite(save_file, &state->context->t_camera->pitch, sizeof(float));
    gzwrite(save_file, &state->context->t_camera->yaw, sizeof(float));

    // Tick State
    gzwrite(save_file, &g_ticksCounter, sizeof(g_ticksCounter));
    gzwrite(save_file, &elapsedRealTime, sizeof(elapsedRealTime));
    gzwrite(save_file, &ticksDayCounter, sizeof(ticksDayCounter));

    // World State
    LevelMap* t_map = &Level::getInstance()->map;
    gzwrite(save_file, &t_map->width, sizeof(t_map->width));
    gzwrite(save_file, &t_map->length, sizeof(t_map->length));
    gzwrite(save_file, &t_map->height, sizeof(t_map->height));
    gzwrite(save_file, &t_map->spawnX, sizeof(t_map->spawnX));
    gzwrite(save_file, &t_map->spawnY, sizeof(t_map->spawnY));
    gzwrite(save_file, &t_map->spawnZ, sizeof(t_map->spawnZ));

    uint32_t worldSize = OVERWORLD_SIZE;
    gzwrite(save_file, &worldSize, sizeof(worldSize));

    gzwrite(save_file, t_map->blocks, sizeof(t_map->blocks));
    gzwrite(save_file, t_map->lightData, sizeof(t_map->lightData));
    gzwrite(save_file, t_map->metaData, sizeof(t_map->metaData));

    gzclose(save_file);
  }
}

void SaveManager::LoadSavedGame(StateGamePlay* state, const char* fullPath) {
  state->world->resetWorldData();
  gzFile save_file = gzopen(fullPath, "rb");

  if (save_file) {
    gzrewind(save_file);

    // Save Version
    int version = 0;
    gzread(save_file, &version, sizeof(int));
    TYRA_LOG("VERSION: ", version);

    if (version == 1) {
      SaveManager::LoadSavedGameV1(state, save_file);
    } else if (version == 2) {
      SaveManager::LoadSavedGameV2(state, save_file);
    } else if (version == 3) {
      SaveManager::LoadSavedGameV3(state, save_file);
    }

    gzclose(save_file);
  }
}

void SaveManager::LoadSavedGameV1(StateGamePlay* state,
                                  const gzFile& save_file) {
  gzrewind(save_file);

  // Save Version
  int version = 0;
  gzread(save_file, &version, sizeof(int));

  NewGameOptions* gameOptions = state->world->getWorldOptions();

  // World seed
  gzread(save_file, &gameOptions->seed, sizeof(uint32_t));

  // Game mode
  gzread(save_file, &gameOptions->gameMode, sizeof(uint8_t));

  // World name
  uint16_t worldNameSize;
  gzread(save_file, &worldNameSize, sizeof(worldNameSize));
  gameOptions->name.resize(worldNameSize / sizeof(char));
  gzread(save_file, gameOptions->name.data(), worldNameSize);

  // World draw distance (legacy — map to nearest mode)
  u8 legacyDrawDistance;
  gzread(save_file, &legacyDrawDistance, sizeof(u8));
  gameOptions->drawDistanceMode = mapLegacyDrawDistance(legacyDrawDistance);

  // World initial time
  gzread(save_file, &gameOptions->initialTime, sizeof(float));

  // World type
  uint8_t worldType;
  gzread(save_file, &worldType, sizeof(uint8_t));
  gameOptions->type = static_cast<WorldType>(worldType);

  // Texture Pack
  uint16_t texturePackSize = 0;
  gzread(save_file, &texturePackSize, sizeof(texturePackSize));
  gameOptions->texturePack.resize(texturePackSize / sizeof(char));
  gzread(save_file, gameOptions->texturePack.data(), texturePackSize);

  // Player position
  Vec4 playerPos;
  gzread(save_file, &playerPos, sizeof(Vec4));

  // PATCH: Fix for v1 save files that missed the w component
  playerPos.w = 1.0f;

  state->player->setPosition(Vec4(playerPos));
  state->world->setSavedSpawnArea(playerPos);

  // TODO: add hot inventory state to save file;

  // Camera direction
  gzread(save_file, &state->context->t_camera->pitch, sizeof(float));
  gzread(save_file, &state->context->t_camera->yaw, sizeof(float));

  // Tick State
  gzread(save_file, &g_ticksCounter, sizeof(g_ticksCounter));
  gzread(save_file, &elapsedRealTime, sizeof(elapsedRealTime));
  gzread(save_file, &ticksDayCounter, sizeof(ticksDayCounter));

  // World State
  LevelMap* t_map = &state->plevel->map;
  gzread(save_file, &t_map->width, sizeof(t_map->width));
  gzread(save_file, &t_map->length, sizeof(t_map->length));
  gzread(save_file, &t_map->height, sizeof(t_map->height));
  gzread(save_file, &t_map->spawnX, sizeof(t_map->spawnX));
  gzread(save_file, &t_map->spawnY, sizeof(t_map->spawnY));
  gzread(save_file, &t_map->spawnZ, sizeof(t_map->spawnZ));

  uint32_t worldSize = 0;
  gzread(save_file, &worldSize, sizeof(worldSize));
  gzread(save_file, t_map->blocks, sizeof(t_map->blocks));
  gzread(save_file, t_map->lightData, sizeof(t_map->lightData));
  gzread(save_file, t_map->metaData, sizeof(t_map->metaData));
}

void SaveManager::LoadSavedGameV2(StateGamePlay* state,
                                  const gzFile& save_file) {
  gzrewind(save_file);

  // Save Version
  int version = 0;
  gzread(save_file, &version, sizeof(int));

  NewGameOptions* gameOptions = state->world->getWorldOptions();

  // World seed
  gzread(save_file, &gameOptions->seed, sizeof(uint32_t));

  // Game mode
  gzread(save_file, &gameOptions->gameMode, sizeof(uint8_t));

  // World name
  uint16_t worldNameSize;
  gzread(save_file, &worldNameSize, sizeof(worldNameSize));
  gameOptions->name.resize(worldNameSize / sizeof(char));
  gzread(save_file, gameOptions->name.data(), worldNameSize);

  // World draw distance (legacy — map to nearest mode)
  u8 legacyDrawDistance;
  gzread(save_file, &legacyDrawDistance, sizeof(u8));
  gameOptions->drawDistanceMode = mapLegacyDrawDistance(legacyDrawDistance);

  // World initial time
  gzread(save_file, &gameOptions->initialTime, sizeof(float));

  // World type
  uint8_t worldType;
  gzread(save_file, &worldType, sizeof(uint8_t));
  gameOptions->type = static_cast<WorldType>(worldType);

  // Texture Pack
  uint16_t texturePackSize = 0;
  gzread(save_file, &texturePackSize, sizeof(texturePackSize));
  gameOptions->texturePack.resize(texturePackSize / sizeof(char));
  gzread(save_file, gameOptions->texturePack.data(), texturePackSize);

  // Player position
  Vec4 playerPos;
  gzread(save_file, &playerPos.xyzw, sizeof(float) * 4);
  state->player->setPosition(Vec4(playerPos.xyzw));
  state->world->setSavedSpawnArea(playerPos);

  // TODO: add hot inventory state to save file;

  // Camera direction
  gzread(save_file, &state->context->t_camera->pitch, sizeof(float));
  gzread(save_file, &state->context->t_camera->yaw, sizeof(float));

  // Tick State
  gzread(save_file, &g_ticksCounter, sizeof(g_ticksCounter));
  gzread(save_file, &elapsedRealTime, sizeof(elapsedRealTime));
  gzread(save_file, &ticksDayCounter, sizeof(ticksDayCounter));

  // World State
  LevelMap* t_map = &state->plevel->map;
  gzread(save_file, &t_map->width, sizeof(t_map->width));
  gzread(save_file, &t_map->length, sizeof(t_map->length));
  gzread(save_file, &t_map->height, sizeof(t_map->height));
  gzread(save_file, &t_map->spawnX, sizeof(t_map->spawnX));
  gzread(save_file, &t_map->spawnY, sizeof(t_map->spawnY));
  gzread(save_file, &t_map->spawnZ, sizeof(t_map->spawnZ));

  uint32_t worldSize = 0;
  gzread(save_file, &worldSize, sizeof(worldSize));
  gzread(save_file, t_map->blocks, sizeof(t_map->blocks));
  gzread(save_file, t_map->lightData, sizeof(t_map->lightData));
  gzread(save_file, t_map->metaData, sizeof(t_map->metaData));
}

void SaveManager::LoadSavedGameV3(StateGamePlay* state,
                                  const gzFile& save_file) {
  gzrewind(save_file);

  // Save Version
  int version = 0;
  gzread(save_file, &version, sizeof(int));

  NewGameOptions* gameOptions = state->world->getWorldOptions();

  // World seed
  gzread(save_file, &gameOptions->seed, sizeof(uint32_t));

  // Game mode
  gzread(save_file, &gameOptions->gameMode, sizeof(uint8_t));

  // World name
  uint16_t worldNameSize;
  gzread(save_file, &worldNameSize, sizeof(worldNameSize));
  gameOptions->name.resize(worldNameSize / sizeof(char));
  gzread(save_file, gameOptions->name.data(), worldNameSize);

  // World draw distance mode
  gzread(save_file, &gameOptions->drawDistanceMode, sizeof(u8));

  // World initial time
  gzread(save_file, &gameOptions->initialTime, sizeof(float));

  // World type
  uint8_t worldType;
  gzread(save_file, &worldType, sizeof(uint8_t));
  gameOptions->type = static_cast<WorldType>(worldType);

  // Texture Pack
  uint16_t texturePackSize = 0;
  gzread(save_file, &texturePackSize, sizeof(texturePackSize));
  gameOptions->texturePack.resize(texturePackSize / sizeof(char));
  gzread(save_file, gameOptions->texturePack.data(), texturePackSize);

  // Player position
  Vec4 playerPos;
  gzread(save_file, &playerPos.xyzw, sizeof(float) * 4);
  state->player->setPosition(Vec4(playerPos.xyzw));
  state->world->setSavedSpawnArea(playerPos);

  // TODO: add hot inventory state to save file;

  // Camera direction
  gzread(save_file, &state->context->t_camera->pitch, sizeof(float));
  gzread(save_file, &state->context->t_camera->yaw, sizeof(float));

  // Tick State
  gzread(save_file, &g_ticksCounter, sizeof(g_ticksCounter));
  gzread(save_file, &elapsedRealTime, sizeof(elapsedRealTime));
  gzread(save_file, &ticksDayCounter, sizeof(ticksDayCounter));

  // World State
  LevelMap* t_map = &state->plevel->map;
  gzread(save_file, &t_map->width, sizeof(t_map->width));
  gzread(save_file, &t_map->length, sizeof(t_map->length));
  gzread(save_file, &t_map->height, sizeof(t_map->height));
  gzread(save_file, &t_map->spawnX, sizeof(t_map->spawnX));
  gzread(save_file, &t_map->spawnY, sizeof(t_map->spawnY));
  gzread(save_file, &t_map->spawnZ, sizeof(t_map->spawnZ));

  uint32_t worldSize = 0;
  gzread(save_file, &worldSize, sizeof(worldSize));
  gzread(save_file, t_map->blocks, sizeof(t_map->blocks));
  gzread(save_file, t_map->lightData, sizeof(t_map->lightData));
  gzread(save_file, t_map->metaData, sizeof(t_map->metaData));
}

NewGameOptions* SaveManager::GetNewGameOptionsFromSaveFile(
    const char* fullPath) {
  NewGameOptions* model = new NewGameOptions();

  gzFile save_file = gzopen(fullPath, "rb");

  if (save_file != nullptr) {
    gzrewind(save_file);

    // Save Version
    int version = 0;
    gzread(save_file, &version, sizeof(int));

    // World seed
    gzread(save_file, &model->seed, sizeof(uint32_t));

    // Game mode
    gzread(save_file, &model->gameMode, sizeof(uint8_t));

    // World name
    uint16_t worldNameSize;
    gzread(save_file, &worldNameSize, sizeof(worldNameSize));
    model->name.resize(worldNameSize / sizeof(char));
    gzread(save_file, model->name.data(), worldNameSize);

    // World draw distance mode
    if (version <= 2) {
      // Legacy: read old drawDistance byte and map to mode
      u8 legacyDrawDistance;
      gzread(save_file, &legacyDrawDistance, sizeof(u8));
      model->drawDistanceMode = mapLegacyDrawDistance(legacyDrawDistance);
    } else {
      gzread(save_file, &model->drawDistanceMode, sizeof(u8));
    }

    // World initial time
    gzread(save_file, &model->initialTime, sizeof(float));

    // World type
    uint8_t worldType;
    gzread(save_file, &worldType, sizeof(uint8_t));
    model->type = static_cast<WorldType>(worldType);

    // Texture Pack
    uint16_t texturePackSize = 0;
    gzread(save_file, &texturePackSize, sizeof(texturePackSize));
    model->texturePack.resize(texturePackSize / sizeof(char));
    gzread(save_file, model->texturePack.data(), texturePackSize);

    gzclose(save_file);
  } else {
    TYRA_TRAP("No could not open save file at: ", fullPath);
  }

  return model;
}

void SaveManager::SetSaveInfo(const char* fullPath, SaveInfoModel* target) {
  gzFile save_file = gzopen(fullPath, "rb");
  if (save_file) {
    gzrewind(save_file);

    // Set Version
    int version;
    gzread(save_file, &version, sizeof(int));

    if (version == 0) {
      target->version = 0;
      target->name = std::string(FileUtils::getFilenameWithoutExtension(
          FileUtils::getFilenameFromPath(fullPath)));
    } else if (version >= 1 && version <= 3) {
      target->version = version;

      // World seed
      uint32_t seed;
      gzread(save_file, &seed, sizeof(uint32_t));

      uint8_t gameMode;
      gzread(save_file, &gameMode, sizeof(uint8_t));

      // World name
      uint16_t worldNameSize;
      gzread(save_file, &worldNameSize, sizeof(worldNameSize));
      target->name.resize(worldNameSize / sizeof(char));
      gzread(save_file, target->name.data(), worldNameSize);
    }

    gzclose(save_file);
  } else {
    target->version = 0;
    target->name = std::string(FileUtils::getFilenameWithoutExtension(
        FileUtils::getFilenameFromPath(fullPath)));
  }
}

bool SaveManager::CheckIfSaveExist(const char* fullPath) {
  struct stat buffer;
  return (stat(fullPath, &buffer) == 0);
}

int SaveManager::DeleteSave(const char* fullPath) {
  if (SaveManager::CheckIfSaveExist(fullPath)) return unlink(fullPath);
  return -1;
}
