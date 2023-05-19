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
#include "managers/clouds_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "models/world_light_model.hpp"
#include "models/new_game_model.hpp"

#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/3d/bbox/core_bbox.hpp>
#include <renderer/3d/bbox/bbox.hpp>
#include <math/vec4.hpp>
#include <physics/ray.hpp>
#include <fastmath.h>
#include <draw_sampling.h>
#include "entities/item.hpp"
#include "entities/level.hpp"
#include "models/sfx_block_model.hpp"
#include "debug/debug.hpp"
#include <limits>

// FROM CrossCraft
#include <stdint.h>
#include "entities/level.hpp"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "managers/cross_craft_world_generator.hpp"

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
  SoundManager* t_soundManager;
  BlockManager blockManager;
  ChunckManager chunckManager;
  CloudsManager cloudsManager;
  DayNightCycleManager dayNightCycleManager = DayNightCycleManager();

  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            SoundManager* t_soundManager);
  void update(Player* t_player, const Vec4& camLookPos,
              const Vec4& camPosition);
  void render();
  void generate();
  void generateLight();
  void generateSpawnArea();
  void loadSpawnArea();
  inline const Vec4 getGlobalSpawnArea() const { return this->worldSpawnArea; };
  inline const Vec4 getLocalSpawnArea() const { return this->spawnArea; };
  const std::vector<Block*> getLoadedBlocks();
  void buildInitialPosition();

  // From terrain manager
  const uint32_t getSeed() { return seed; };

  Block* targetBlock = nullptr;

  void updateTargetBlock(const Vec4& camLookPos, const Vec4& camPosition,
                         const std::vector<Chunck*>& chuncks);
  void removeBlock(Block* blockToRemove);
  void putBlock(const Blocks& blockType, Player* t_player);
  inline const u8 validTargetBlock() {
    return this->targetBlock != nullptr && this->targetBlock->isBreakable;
  };

  void setSavedSpawnArea(Vec4 pos);
  const Vec4 defineSpawnArea();
  const Vec4 calcSpawOffset(int bias = 0);
  void buildChunk(Chunck* t_chunck);
  void buildChunkAsync(Chunck* t_chunck, const u8& loading_speed);

  inline u8 isBreakingBLock() { return this->_isBreakingBlock; };
  void breakTargetBlock(const float& deltaTime);
  void stopBreakTargetBlock();

  LevelMap* terrain;

  void setDrawDistace(const u8& drawDistanceInChunks);
  inline const u8 getDrawDistace() { return worldOptions.drawDistance; };
  inline NewGameOptions* getWorldOptions() { return &worldOptions; };

  void resetWorldData();
  void reloadWorldArea(const Vec4& position);

 private:
  MinecraftPipeline mcPip;
  StaticPipeline stapip;
  std::vector<Block*> loadedBlocks;
  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4 lastPlayerPosition;
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
  void updateNeighBorsChunksByModdedPosition(const Vec4& pos);
  void addChunkToLoadAsync(Chunck* t_chunck);
  void addChunkToUnloadAsync(Chunck* t_chunck);
  void renderBlockDamageOverlay();
  void renderTargetBlockHitbox(Block* targetBlock);
  void updateLightModel();
  void sortChunksToLoad(const Vec4& currentPlayerPos);
  inline void setIntialTime() { g_ticksCounter = worldOptions.initialTime; };

  // From terrain manager
  Ray ray;
  ItemRepository* t_itemRepository;

  // Breaking control
  u8 _isBreakingBlock = false;
  float breaking_time_pessed = 0.0F;
  float lastTimePlayedBreakingSfx = 0.0F;

  Vec4 minWorldPos;
  Vec4 maxWorldPos;
  BBox* rawBlockBbox;

  uint32_t seed;

  u8 isBlockAtChunkBorder(const Vec4* blockOffset, const Vec4* chunkMinOffset,
                          const Vec4* chunkMaxOffset);
  unsigned int getIndexByOffset(int x, int y, int z);

  /**
   * @brief Update the visible block faces
   * @return false if the block is completely hidden
   *
   */
  int getBlockVisibleFaces(const Vec4* t_blockOffset);
  int getWaterBlockVisibleFaces(const Vec4* t_blockOffset);

  inline bool isBlockTransparentAtPosition(const float& x, const float& y,
                                           const float& z);
  inline bool isAirAtPosition(const float& x, const float& y, const float& z);

  inline bool isTopFaceVisible(const Vec4* t_blockOffset);
  inline bool isBottomFaceVisible(const Vec4* t_blockOffset);
  inline bool isLeftFaceVisible(const Vec4* t_blockOffset);
  inline bool isRightFaceVisible(const Vec4* t_blockOffset);
  inline bool isFrontFaceVisible(const Vec4* t_blockOffset);
  inline bool isBackFaceVisible(const Vec4* t_blockOffset);

  void calcRawBlockBBox(MinecraftPipeline* mcPip);
  void getBlockMinMax(Block* t_block);

  void playPutBlockSound(const Blocks& blockType);
  void playDestroyBlockSound(const Blocks& blockType);
  void playBreakingBlockSound(const Blocks& blockType);
};

static Level level;

// FROM CrossCraft
void CrossCraft_World_Init(const uint32_t& seed);
void CrossCraft_World_Deinit();

/**
 * Allocate memory for an empty map with a selected world size.
 */
void AllocateMapData();

/**
 * This method should ONLY be used by a clien t in single-player or a server for
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
void singleCheck(uint16_t x, uint16_t z);
void updateRemove();
void updateRemove(uint32_t* updateIDs);
void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel);
void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
                     uint32_t* updateIDs);

void updateSunlight();
void propagateSunLightAddBFSQueue();
void floodFillSunlightAdd(uint16_t x, uint16_t y, uint16_t z,
                          u8 nextLightValue);
void addSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);

void propagateSunlightRemovalQueue();
void floodFillSunlightRemove(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void removeSunLight(uint16_t x, uint16_t y, uint16_t z);
void removeSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
bool inline IsTransparent(Blocks block) {
  return block == Blocks::AIR_BLOCK || block == Blocks::OAK_LEAVES_BLOCK ||
         block == Blocks::WATER_BLOCK || block == Blocks::GLASS_BLOCK;
};