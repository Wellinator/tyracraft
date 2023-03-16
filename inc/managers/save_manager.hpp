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
#include <stdint.h>
#include <vector>
#include <3libs/gzip/compress.hpp>
#include <3libs/gzip/config.hpp>
#include <3libs/gzip/decompress.hpp>
#include <3libs/gzip/utils.hpp>
#include <3libs/gzip/version.hpp>

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

    TYRA_LOG("Saving World Options...");
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

    std::ostringstream tempBlocksBuffer;
    for (size_t i = 0; i < OVERWORLD_SIZE; i++)
      tempBlocksBuffer << std::to_string(t_map->blocks[i]);
    jsonfile["worldLevel"]["map"]["blocks"] = tempBlocksBuffer.str();

    TYRA_LOG("Compressing data...");
    auto rawData = jsonfile.dump();
    std::string compressed_data =
        gzip::compress(rawData.data(), rawData.size(), Z_BEST_COMPRESSION);

    std::ofstream saveFile(fullPath);
    saveFile << compressed_data;
    saveFile.close();
  };

  static void LoadSavedGame(StateGamePlay* state, const char* fullPath) {
    TYRA_LOG("Decompressing data...");
    std::ifstream saveFile(fullPath);
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

    TYRA_LOG("Loading World Options...");
    NewGameOptions* gameOptions = state->world->getWorldOptions();
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
    g_ticksCounter = savedData["tickState"]["g_ticksCounter"].get<float>();
    elapsedRealTime = savedData["tickState"]["elapsedRealTime"].get<double>();
    ticksDayCounter = savedData["tickState"]["ticksDayCounter"].get<uint16_t>();

    TYRA_LOG("Loading World State...");
    LevelMap* t_map = CrossCraft_World_GetMapPtr();
    t_map->width = savedData["worldLevel"]["map"]["width"].get<uint16_t>();
    t_map->length = savedData["worldLevel"]["map"]["length"].get<uint16_t>();
    t_map->height = savedData["worldLevel"]["map"]["height"].get<uint16_t>();
    t_map->spawnX = savedData["worldLevel"]["map"]["spawnX"].get<uint16_t>();
    t_map->spawnY = savedData["worldLevel"]["map"]["spawnY"].get<uint16_t>();
    t_map->spawnZ = savedData["worldLevel"]["map"]["spawnZ"].get<uint16_t>();
    t_map->blocks = reinterpret_cast<uint8_t*>(const_cast<char*>(
        savedData["worldLevel"]["map"]["blocks"].get<std::string>().c_str()));
  };
};
