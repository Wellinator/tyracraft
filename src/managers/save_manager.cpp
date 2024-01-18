#include "managers/save_manager.hpp"

void SaveManager::SaveGame(StateGamePlay* state, const char* fullPath) {
  gzFile save_file = gzopen(fullPath, "wb");

  if (save_file != nullptr) {
    const int save_version = 1;

    // Save version
    gzwrite(save_file, &save_version, sizeof(int) * 1);

    // World seed
    gzwrite(save_file, &state->world->getWorldOptions()->seed,
            sizeof(uint32_t));

    // World name
    uint16_t worldNameSize =
        state->world->getWorldOptions()->name.size() * sizeof(char);
    gzwrite(save_file, &worldNameSize, sizeof(worldNameSize));
    gzwrite(save_file, state->world->getWorldOptions()->name.data(),
            worldNameSize);

    // World draw distance
    gzwrite(save_file, &state->world->getWorldOptions()->drawDistance,
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
    gzwrite(save_file, state->player->getPosition(), sizeof(Vec4));

    // TODO: add hot inventory state to save file;

    // Camera direction
    gzwrite(save_file, &state->context->t_camera->pitch, sizeof(float));
    gzwrite(save_file, &state->context->t_camera->yaw, sizeof(float));

    // Tick State
    gzwrite(save_file, &g_ticksCounter, sizeof(g_ticksCounter));
    gzwrite(save_file, &elapsedRealTime, sizeof(elapsedRealTime));
    gzwrite(save_file, &ticksDayCounter, sizeof(ticksDayCounter));

    // World State
    LevelMap* t_map = CrossCraft_World_GetMapPtr();
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

    NewGameOptions* gameOptions = state->world->getWorldOptions();

    // World seed
    gzread(save_file, &gameOptions->seed, sizeof(uint32_t));

    // World name
    uint16_t worldNameSize = 0;
    gzread(save_file, &worldNameSize, sizeof(worldNameSize));
    gameOptions->name.resize(worldNameSize / sizeof(char));
    gzread(save_file, gameOptions->name.data(), worldNameSize);

    // World draw distance
    gzread(save_file, &gameOptions->drawDistance, sizeof(u8));

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
    state->player->getPosition()->set(playerPos);

    // TODO: add hot inventory state to save file;

    // Camera direction
    gzread(save_file, &state->context->t_camera->pitch, sizeof(float));
    gzread(save_file, &state->context->t_camera->yaw, sizeof(float));

    // Tick State
    gzread(save_file, &g_ticksCounter, sizeof(g_ticksCounter));
    gzread(save_file, &elapsedRealTime, sizeof(elapsedRealTime));
    gzread(save_file, &ticksDayCounter, sizeof(ticksDayCounter));

    // World State
    LevelMap* t_map = CrossCraft_World_GetMapPtr();
    gzread(save_file, &t_map->width, sizeof(t_map->width));
    gzread(save_file, &t_map->length, sizeof(t_map->length));
    gzread(save_file, &t_map->height, sizeof(t_map->height));
    gzread(save_file, &t_map->spawnX, sizeof(t_map->spawnX));
    gzread(save_file, &t_map->spawnY, sizeof(t_map->spawnY));
    gzread(save_file, &t_map->spawnZ, sizeof(t_map->spawnZ));

    state->world->setSavedSpawnArea(*state->player->getPosition());

    uint32_t worldSize = 0;
    gzread(save_file, &worldSize, sizeof(worldSize));
    gzread(save_file, t_map->blocks, sizeof(t_map->blocks));
    gzread(save_file, t_map->lightData, sizeof(t_map->lightData));
    gzread(save_file, t_map->metaData, sizeof(t_map->metaData));

    gzclose(save_file);
  }
}

NewGameOptions* SaveManager::GetNewGameOptionsFromSaveFile(
    const char* fullPath) {
  NewGameOptions* model = new NewGameOptions();

  gzFile save_file = gzopen(fullPath, "rb");
  gzrewind(save_file);

  if (save_file != nullptr) {
    // Save Version
    int version = 0;
    gzread(save_file, &version, sizeof(int));
    TYRA_LOG("VERSION: ", version);

    // World seed
    gzread(save_file, &model->seed, sizeof(uint32_t));

    // World name
    uint16_t worldNameSize = 0;
    gzread(save_file, &worldNameSize, sizeof(worldNameSize));
    model->name.resize(worldNameSize / sizeof(char));
    gzread(save_file, model->name.data(), worldNameSize);

    // World draw distance
    gzread(save_file, &model->drawDistance, sizeof(u8));

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

bool SaveManager::CheckIfSaveExist(const char* fullPath) {
  struct stat buffer;
  return (stat(fullPath, &buffer) == 0);
}