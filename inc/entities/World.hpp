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
#include <unordered_set>
#include <queue>
#include <deque>
#include <algorithm>
#include "managers/chunck_manager.hpp"
#include "managers/clouds_manager.hpp"
#include "managers/particle_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "memory-monitor/memory_monitor.hpp"
#include "models/world_light_model.hpp"
#include "models/new_game_model.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_config_model.hpp"

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
#include <stdint-gcc.h>

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

struct Node {
  uint16_t x, y, z;
  Node(uint16_t lx, uint16_t ly, uint16_t lz, uint16_t l)
      : x(lx), y(ly), z(lz), val(l) {}
  uint16_t val;
};

class World {
 public:
  World(const NewGameOptions& options);
  ~World();

  Renderer* t_renderer;
  SoundManager* t_soundManager;
  BlockManager blockManager;
  ChunckManager chunckManager;
  CloudsManager cloudsManager;
  ParticlesManager particlesManager;
  DayNightCycleManager dayNightCycleManager;

  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            SoundManager* t_soundManager);
  void update(Player* t_player, Camera* t_camera, const float deltaTime);
  void render();
  void generate();
  void generateLight();
  void propagateLiquids();
  void generateSpawnArea();
  void loadSpawnArea();
  inline const Vec4 getGlobalSpawnArea() const { return this->worldSpawnArea; };
  inline const Vec4 getLocalSpawnArea() const { return this->spawnArea; };
  void buildInitialPosition();

  // From terrain manager
  const uint32_t getSeed() { return seed; };

  Block* targetBlock = nullptr;

  void updateTargetBlock(Camera* t_camera, Player* t_player);
  void removeBlock(Block* blockToRemove);

  void putBlock(const Blocks& blockType, Player* t_player,
                const float cameraYaw);
  void putTorchBlock(const PlacementDirection placementDirection,
                     const float cameraYaw, Vec4 blockOffset);
  void putDefaultBlock(const Blocks blockToPlace, Player* t_player,
                       const float cameraYaw, Vec4 blockOffset);

  inline const u8 validTargetBlock() {
    return targetBlock != nullptr && targetBlock->isBreakable;
  };

  void setSavedSpawnArea(Vec4 pos);
  const Vec4 defineSpawnArea();
  const Vec4 calcSpawOffset(int bias = 0);
  void buildChunk(Chunck* t_chunck);
  void buildChunkAsync(Chunck* t_chunck, const u8& loading_speed);

  inline u8 isBreakingBLock() { return this->_isBreakingBlock; };
  void breakTargetBlock(const float& deltaTime);
  void breakTargetBlockInCreativeMode(const float& deltaTime);
  void stopBreakTargetBlock();

  LevelMap* terrain;

  void setDrawDistace(const u8& drawDistanceInChunks);
  inline const u8 getDrawDistace() { return worldOptions.drawDistance; };
  inline NewGameOptions* getWorldOptions() { return &worldOptions; };

  void resetWorldData();
  void reloadWorldArea(const Vec4& position);

  inline size_t getChunksToLoadCount() { return tempChuncksToLoad.size(); };
  inline size_t getChunksToUnloadCount() { return tempChuncksToUnLoad.size(); };
  inline size_t getChuncksToUpdateLightCount() {
    return chunckManager.getChuncksToUpdateLightCount();
  };

  inline u8 canBuildChunk() {
    return (get_used_memory() >> 20) < MAX_SAFE_MEMORY_ALLOCATION;
  };

  inline WorldLightModel* getWorldLightModel() { return &worldLightModel; };

 private:
  MinecraftPipeline mcPip;
  StaticPipeline stapip;

  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4 lastPlayerPosition;
  NewGameOptions worldOptions = NewGameOptions();

  int maxLoadPerCall = 1;
  int maxUnloadPerCall = 2;

  std::deque<Chunck*> tempChuncksToLoad;
  std::deque<Chunck*> tempChuncksToUnLoad;
  std::vector<McpipBlock*> overlayData;

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

  inline void setIntialTime() {
    g_ticksCounter = static_cast<int>(worldOptions.initialTime);
  };

  // From terrain manager
  Ray ray;
  ItemRepository* t_itemRepository;

  // Paticles values
  float lastTimeCreatedParticle = 0.0F;

  // Breaking control
  u8 _isBreakingBlock = false;
  float breaking_time_pessed = 0.0F;
  float lastTimePlayedBreakingSfx = 0.0F;

  Vec4 minWorldPos;
  Vec4 maxWorldPos;
  BBox* rawBlockBbox;

  uint32_t seed;

  inline u8 isCrossedBlock(Blocks block_type);
  inline u8 isBlockAtChunkBorder(const Vec4* blockOffset,
                                 const Vec4* chunkMinOffset,
                                 const Vec4* chunkMaxOffset);
  inline u32 getIndexByOffset(int x, int y, int z);

  /**
   * @brief Update the visible block faces
   * @return false if the block is completely hidden
   *
   */
  u8 getBlockVisibleFaces(const Vec4* t_blockOffset);
  u8 getLiquidBlockVisibleFaces(const Vec4* t_blockOffset);

  inline u8 isBlockTransparentAtPosition(const float& x, const float& y,
                                         const float& z);
  inline u8 isAirAtPosition(const float& x, const float& y, const float& z);

  inline u8 isTopFaceVisible(const Vec4* t_blockOffset);
  inline u8 isBottomFaceVisible(const Vec4* t_blockOffset);
  inline u8 isLeftFaceVisible(const Vec4* t_blockOffset);
  inline u8 isRightFaceVisible(const Vec4* t_blockOffset);
  inline u8 isFrontFaceVisible(const Vec4* t_blockOffset);
  inline u8 isBackFaceVisible(const Vec4* t_blockOffset);

  void calcRawBlockBBox(MinecraftPipeline* mcPip);

  void playPutBlockSound(const Blocks& blockType);
  void playDestroyBlockSound(const Blocks& blockType);
  void playBreakingBlockSound(const Blocks& blockType);
  void playFlowingWaterSound();

  void initWorldLightModel();

  // Should be true after propagate 5 blocks;
  u8 hasMachedLimitAdd = false;
  u8 hasMachedLimitRemove = false;

  // Liquids
  uint16_t waterSoundDuration = 4000;
  float waterSoundTimeCounter = 0.0F;
  float lastTimePlayedWaterSound = 0.0F;
  std::queue<Node> waterBfsQueue;
  std::queue<Node> waterRemovalBfsQueue;
  std::queue<Node> lavaBfsQueue;
  std::queue<Node> lavaRemovalBfsQueue;
  std::unordered_set<Chunck*> affectedChunksIdByLiquidPropagation;

  void initLiquidExpansion();
  void checkLiquidPropagation(uint16_t x, uint16_t y, uint16_t z);
  void addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level);
  void addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level,
                 u8 orientation);
  void removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type);
  void removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level);
  void floodFillLiquidAdd(uint16_t x, uint16_t y, uint16_t z, u8 type,
                          u8 nextLevel, u8 orientation);
  void floodFillLiquidRemove(uint16_t x, uint16_t y, uint16_t z, u8 type,
                             u8 level);
  void updateLiquidWater(const float deltaTime);
  void propagateWaterRemovalQueue();
  void propagateWaterAddQueue();
  void updateLiquidLava();
  void propagateLavaRemovalQueue();
  void propagateLavaAddQueue();
  void updateChunksAffectedByLiquidPropagation();
  u8 canPropagateLiquid(uint16_t x, uint16_t y, uint16_t z);
};

bool inline isVegetation(Blocks block) {
  return (u8)block >= (u8)Blocks::GRASS &&
         (u8)block <= (u8)Blocks::DANDELION_FLOWER;
};

bool inline isTransparent(Blocks block) {
  return block == Blocks::AIR_BLOCK || block == Blocks::WATER_BLOCK ||
         block == Blocks::GLASS_BLOCK || block == Blocks::POPPY_FLOWER ||
         block == Blocks::DANDELION_FLOWER || block == Blocks::GRASS ||
         block == Blocks::TORCH;
};

///////////////////////////////////
// Based in Seed of Andromeda    //
///////////////////////////////////
void initSunLight(uint32_t tick);
void addSunLight(uint16_t x, uint16_t y, uint16_t z);
void addSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void removeSunLight(uint16_t x, uint16_t y, uint16_t z);
void removeSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void updateSunlight();
void propagateSunLightAddBFSQueue();
void propagateSunlightRemovalQueue();
void floodFillSunlightAdd(uint16_t x, uint16_t y, uint16_t z,
                          u8 nextLightValue);
void floodFillSunlightRemove(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void checkSunLightAt(uint16_t x, uint16_t y, uint16_t z);

void initBlockLight(BlockManager* blockManager);
void addBlockLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void updateBlockLights();

void removeLight(uint16_t x, uint16_t y, uint16_t z);
void removeLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void floodFillLightAdd(uint16_t x, uint16_t y, uint16_t z, u8 nextLightValue);
void floodFillLightRemove(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
void propagateLightRemovalQueue();
void propagateLightAddQueue();

// FROM CrossCraft
void CrossCraft_World_Init(const uint32_t& seed);
void CrossCraft_World_Deinit();

/**
 * This method should ONLY be used by a clien t in single-player or a server for
 * internal use.
 * @return Returns a pointer to the level
 */
LevelMap* CrossCraft_World_GetMapPtr();

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void CrossCraft_World_GenerateMap(WorldType worldType);
