#pragma once

#include <engine.hpp>
#include <tamtypes.h>
#include <time/timer.hpp>
#include <pad/pad.hpp>
#include <renderer/renderer.hpp>
#include "chunck.hpp"
#include "entities/player/player.hpp"
#include "entities/Block.hpp"
#include "managers/items_repository.hpp"
#include "constants.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include <vector>
#include <algorithm>
#include "managers/chunck_manager.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "models/world_light_model.hpp"
#include "models/new_game_model.hpp"

// FROM CrossCraft
#include <stdint.h>
#include "level.hpp"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::Pad;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Vec4;

class World {
 public:
  World(const NewGameOptions& options);
  ~World();

  Renderer* t_renderer;
  TerrainManager* terrainManager;
  BlockManager* blockManager;
  ChunckManager* chunckManager;
  DayNightCycleManager dayNightCycleManager = DayNightCycleManager();

  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            SoundManager* t_soundManager);
  void update(Player* t_player, const Vec4& camLookPos,
              const Vec4& camPosition);
  void render();
  inline const Vec4 getGlobalSpawnArea() const { return this->worldSpawnArea; };
  inline const Vec4 getLocalSpawnArea() const { return this->spawnArea; };
  const std::vector<Block*> getLoadedBlocks();
  void buildInitialPosition();
  inline const int getSeed() { return this->terrainManager->getSeed(); };

 private:
  MinecraftPipeline mcPip;
  StaticPipeline stapip;
  std::vector<Block*> loadedBlocks;
  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4* lastPlayerPosition = new Vec4();
  u8 framesCounter = 0;
  NewGameOptions worldOptions = NewGameOptions();

  int maxLoadPerCall = 1;
  int maxUnloadPerCall = 2;

  std::vector<Chunck*> tempChuncksToLoad;
  std::vector<Chunck*> tempChuncksToUnLoad;
  std::vector<McpipBlock*> overlayData;
  std::vector<Vec4> lightsPositions = {Vec4(0, 0, 0)};

  WorldLightModel worldLightModel;

  void updateChunkByPlayerPosition(Player* player);
  void scheduleChunksNeighbors(Chunck* t_chunck, const Vec4 currentPlayerPos,
                               u8 force_loading = 0);
  void loadScheduledChunks();
  void unloadScheduledChunks();
  void updateNeighBorsChunksByModdedBlock(Block* changedBlock);
  void reloadChangedChunkByRemovedBlock();
  void reloadChangedChunkByPutedBlock();
  void addChunkToLoadAsync(Chunck* t_chunck);
  void addChunkToUnloadAsync(Chunck* t_chunck);
  void renderBlockDamageOverlay();
  void renderTargetBlockHitbox(Block* targetBlock);
  void updateLightModel();
  void sortChunksToLoad(const Vec4& currentPlayerPos);
  inline void setIntialTime() { g_ticksCounter = worldOptions.initialTime; };
};

static Level level;

// FROM CrossCraft
void CrossCraft_World_Init();
void CrossCraft_World_Deinit();

/**
 * Creates an empty map with a selected world size.
 * @param size Size Constant
 */
void CrossCraft_World_Create_Map(uint8_t size);

/**
 * This method should ONLY be used by a client in single-player or a server for
 * internal use.
 * @return Returns a pointer to the level
 */
LevelMap* CrossCraft_World_GetMapPtr();

/**
 * @brief Spawn the player into the world
 */
void CrossCraft_World_Spawn();

/**
 * @brief Tries to load a world
 * @returns If the world was loaded
 */
// bool CrossCraft_World_TryLoad(uint8_t slot, const char* prefix);

typedef enum {
  WORLD_TYPE_ORIGINAL = 0,
  WORLD_TYPE_FLAT = 1,
  WORLD_TYPE_ISLAND = 2,
  WORLD_TYPE_WOODS = 3,
  WORLD_TYPE_FLOATING = 4
} WorldType;

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void CrossCraft_World_GenerateMap(WorldType worldType);

void CrossCraft_World_PropagateSunLight(uint32_t tick);
bool CrossCraft_World_CheckSunLight(uint16_t x, uint16_t y, uint16_t z);

void CrossCraft_World_AddLight(uint16_t x, uint16_t y, uint16_t z,
                               uint16_t light, uint32_t* updateIDs);
void CrossCraft_World_RemoveLight(uint16_t x, uint16_t y, uint16_t z,
                                  uint16_t light, uint32_t* updateIDs);

void updateSpread(uint32_t* updateIDs);
void updateSpread();
void checkAddID();
void updateID(uint16_t x, uint16_t z, uint32_t* updateIDs);
void propagate(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
               uint32_t* updateIDs);
void propagate(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel);
void updateSunlightRemove();
void updateSunlight();
void updateRemove();
void updateRemove(uint32_t* updateIDs);
void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel);
void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
                     uint32_t* updateIDs);


