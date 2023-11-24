#pragma once
#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"
#include <string>
#include "managers/tick_manager.hpp"
#include "states/game_play/state_game_play.hpp"
#include "models/save_game_model.hpp"
#include "entities/World.hpp"
#include "entities/level.hpp"
#include <3libs/nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <3libs/gzip/compress.hpp>
#include <3libs/gzip/config.hpp>
#include <3libs/gzip/decompress.hpp>
#include <3libs/gzip/utils.hpp>
#include <3libs/gzip/version.hpp>
#include <stdint.h>

using Tyra::FileUtils;
using json = nlohmann::json;

class SaveManager {
 public:
  static void SaveGame(StateGamePlay* state, const char* fullPath) {
    json jsonfile;

    TYRA_LOG("Saving player position...");
    jsonfile["playerPosition"]["x"] = state->player->getPosition()->x;
    jsonfile["playerPosition"]["y"] = state->player->getPosition()->y;
    jsonfile["playerPosition"]["z"] = state->player->getPosition()->z;

    TYRA_LOG("Saving camera params...");
    jsonfile["cameraParams"]["pitch"] = state->context->t_camera->pitch;
    jsonfile["cameraParams"]["yaw"] = state->context->t_camera->yaw;

    TYRA_LOG("Saving World Options...");
    jsonfile["gameOptions"]["name"] = state->world->getWorldOptions()->name;
    jsonfile["gameOptions"]["seed"] = state->world->getWorldOptions()->seed;
    jsonfile["gameOptions"]["drawDistance"] =
        state->world->getWorldOptions()->drawDistance;
    jsonfile["gameOptions"]["initialTime"] =
        state->world->getWorldOptions()->initialTime;
    jsonfile["gameOptions"]["type"] =
        (uint8_t)state->world->getWorldOptions()->type;
    jsonfile["gameOptions"]["texturePack"] =
        state->world->getWorldOptions()->texturePack;

    TYRA_LOG("Saving tick state...");
    jsonfile["tickState"]["g_ticksCounter"] = g_ticksCounter;
    jsonfile["tickState"]["elapsedRealTime"] = elapsedRealTime;
    jsonfile["tickState"]["ticksDayCounter"] = ticksDayCounter;

    TYRA_LOG("Saving World State...");
    LevelMap* t_map = CrossCraft_World_GetMapPtr();
    jsonfile["worldLevel"]["map"]["width"] = t_map->width;
    jsonfile["worldLevel"]["map"]["length"] = t_map->length;
    jsonfile["worldLevel"]["map"]["height"] = t_map->height;
    jsonfile["worldLevel"]["map"]["spawnX"] = t_map->spawnX;
    jsonfile["worldLevel"]["map"]["spawnY"] = t_map->spawnY;
    jsonfile["worldLevel"]["map"]["spawnZ"] = t_map->spawnZ;

    std::string tempBlocksBuffer;
    for (size_t i = 0; i < OVERWORLD_SIZE; i++) {
      tempBlocksBuffer.push_back(t_map->blocks[i] + '0');
    }
    jsonfile["worldLevel"]["map"]["blocks"] = tempBlocksBuffer;

    TYRA_LOG("Compressing data...");
    auto rawData = jsonfile.dump();
    std::string compressed_data =
        gzip::compress(rawData.data(), rawData.size());

    std::ofstream saveFile(fullPath);
    saveFile << compressed_data;
    saveFile.close();

    tempBlocksBuffer.clear();
    tempBlocksBuffer.shrink_to_fit();
  };

  static void LoadSavedGame(StateGamePlay* state, const char* fullPath) {
    TYRA_LOG("Reseting world data...");
    state->world->resetWorldData();

    TYRA_LOG("Reading save file from : ", fullPath);
    std::ifstream saveFile(fullPath);
    if (saveFile) {
      TYRA_LOG("Decompressing data...");
      std::string compressed_data((std::istreambuf_iterator<char>(saveFile)),
                                  std::istreambuf_iterator<char>());
      std::string decompressed_data =
          gzip::decompress(compressed_data.data(), compressed_data.size());
      json savedData = json::parse(decompressed_data);

      TYRA_LOG("Loading player Position...");
      state->player->getPosition()->set(
          savedData["playerPosition"]["x"].get<float>(),
          savedData["playerPosition"]["y"].get<float>(),
          savedData["playerPosition"]["z"].get<float>());

      // TODO: add hot inventory state to save file;

      TYRA_LOG("Loading camera params...");
      state->context->t_camera->pitch =
          savedData["cameraParams"]["pitch"].get<float>();
      state->context->t_camera->yaw =
          savedData["cameraParams"]["yaw"].get<float>();

      TYRA_LOG("Loading world pptions...");
      NewGameOptions* gameOptions = state->world->getWorldOptions();
      gameOptions->name = savedData["gameOptions"]["name"].get<std::string>();
      gameOptions->seed = savedData["gameOptions"]["seed"].get<uint32_t>();
      gameOptions->drawDistance =
          savedData["gameOptions"]["drawDistance"].get<u8>();
      gameOptions->initialTime =
          savedData["gameOptions"]["initialTime"].get<float>();
      gameOptions->type = savedData["gameOptions"]["type"].get<WorldType>();
      gameOptions->texturePack =
          savedData["gameOptions"]["texturePack"].get<std::string>();

      TYRA_LOG("Loading tick state...");
      // These are global scope variables;
      g_ticksCounter = savedData["tickState"]["g_ticksCounter"].get<uint32_t>();
      elapsedRealTime = savedData["tickState"]["elapsedRealTime"].get<double>();
      ticksDayCounter =
          savedData["tickState"]["ticksDayCounter"].get<uint16_t>();

      TYRA_LOG("Loading world state...");
      LevelMap* t_map = CrossCraft_World_GetMapPtr();
      t_map->width = savedData["worldLevel"]["map"]["width"].get<uint16_t>();
      t_map->length = savedData["worldLevel"]["map"]["length"].get<uint16_t>();
      t_map->height = savedData["worldLevel"]["map"]["height"].get<uint16_t>();
      t_map->spawnX = savedData["worldLevel"]["map"]["spawnX"].get<uint16_t>();
      t_map->spawnY = savedData["worldLevel"]["map"]["spawnY"].get<uint16_t>();
      t_map->spawnZ = savedData["worldLevel"]["map"]["spawnZ"].get<uint16_t>();

      state->world->setSavedSpawnArea(*state->player->getPosition());

      TYRA_LOG("Loading blocks data...");
      const char* tempBLocksBuffer =
          savedData["worldLevel"]["map"]["blocks"].get<std::string>().c_str();

      for (size_t i = 0; i < OVERWORLD_SIZE; i++) {
        uint8_t number = tempBLocksBuffer[i] - 48;
        t_map->blocks[i] = number;
      }

      // TYRA_LOG("Reloading world data...");
      // state->world->reloadWorldArea(*state->player->getPosition());
    } else {
      TYRA_ERROR("Save file not found at: ", fullPath);
    }
  };

  static NewGameOptions* GetNewGameOptionsFromSaveFile(const char* fullPath) {
    NewGameOptions* model = new NewGameOptions();

    TYRA_LOG("Decompressing data...");
    std::ifstream saveFile(fullPath);
    if (saveFile) {
      std::string compressed_data((std::istreambuf_iterator<char>(saveFile)),
                                  std::istreambuf_iterator<char>());
      std::string decompressed_data =
          gzip::decompress(compressed_data.data(), compressed_data.size());
      json savedData = json::parse(decompressed_data);

      TYRA_LOG("Loading world pptions...");
      model->seed = savedData["gameOptions"]["seed"].get<uint32_t>();
      model->drawDistance = savedData["gameOptions"]["drawDistance"].get<u8>();
      model->initialTime = savedData["gameOptions"]["initialTime"].get<float>();
      model->type = savedData["gameOptions"]["type"].get<WorldType>();
      model->texturePack =
          savedData["gameOptions"]["texturePack"].get<std::string>();
    }

    return model;
  }

  static bool CheckIfSaveExist(const char* fullPath) {
    struct stat buffer;
    return (stat(fullPath, &buffer) == 0);
  }
};