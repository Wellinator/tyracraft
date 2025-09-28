#pragma once

#include <engine.hpp>
#include <tamtypes.h>
#include <time/timer.hpp>
#include <pad/pad.hpp>
#include <renderer/renderer.hpp>
#include "chunk.hpp"
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
#include "managers/chunk_manager.hpp"
#include "managers/clouds_manager.hpp"
#include "managers/particle/particle_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "managers/mob/mob_manager.hpp"
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
  World(const NewGameOptions& options, Level* level);
  ~World();

  Level* pLevel;
  Renderer* t_renderer;
  MobManager mobManager;
  BlockManager blockManager;
  ChunkManager chunkManager;
  CloudsManager cloudsManager;
  ParticlesManager particlesManager;
  DayNightCycleManager dayNightCycleManager;

  void init(Renderer* t_renderer, ItemRepository* itemRepository);
  void fixedUpdate(Player* t_player, Camera* t_camera,
                   const float fixedDeltaTime);
  void update(Player* t_player, Camera* t_camera, const float deltaTime);
  void tick(Player* t_player, Camera* t_camera);
  void renderOpaque();
  void renderTransparent();
  void renderBlockDamageOverlay();
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

  TargetedFace getTargetedFace();

  void placeBlockAt(const Blocks& blockType, const Vec4& blockOffset);
  void removeBlock(Block* blockToRemove);
  void mergeSlabs(const Blocks& slabToPlace, Player* t_player,
                  const Vec4& offsetToMerge);

  bool putBlock(const Blocks& blockType, Player* t_player);
  bool putTorchBlock();
  bool putSlab(const Blocks& slabToPlace, Player* t_player);
  bool putDefaultBlock(const Blocks blockToPlace, Player* t_player);

  inline const u8 validTargetBlock() {
    return targetBlock != nullptr && targetBlock->isBreakable();
  };

  void setSavedSpawnArea(Vec4 pos);
  const Vec4 defineSpawnArea();
  const Vec4 calcSpawOffset(int bias = 0);
  void rebuildChunkNeighbors(Chunk* t_chunk, Vec4* moddedOffset);

  inline u8 isBreakingBlock() { return this->_isBreakingBlock; };
  void breakTargetBlock(const float& deltaTime);
  void breakTargetBlockInCreativeMode(const float& deltaTime);
  void stopBreakTargetBlock();

  void setDrawDistance(const u8& drawDistanceInChunks);
  inline const u8 getDrawDistace() { return worldOptions.drawDistance; };

  void resetWorldData();
  void reloadWorldArea(const Vec4& position);

  inline size_t getChunksToLoadCount() { return tempChunksToLoad.size(); };
  inline size_t getChunksToUnloadCount() { return tempChunksToUnLoad.size(); };
  inline size_t getChunksToUpdateLightCount() {
    return chunkManager.getChunksToUpdateLightCount();
  };

  inline u8 canBuildChunk() {
#ifdef DEBUG_MODE
    return (get_used_memory() >> 20) < MAX_SAFE_MEMORY_ALLOCATION;
#else
    return true;
#endif  // end if DEBUG_MODE
  };

  inline WorldLightModel* getWorldLightModel() { return &worldLightModel; };

  const uint32_t getSeed() const { return seed; };
  void setSeed(const uint32_t& newSeed) { seed = newSeed; };

  inline NewGameOptions* getWorldOptions() { return &worldOptions; };
  const NewGameOptions& getWorldOptions() const { return worldOptions; };
  void setWorldOptions(const NewGameOptions& options) {
    worldOptions = options;
  };

 private:
  StaticPipeline stapip;

  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4 lastPlayerPosition;
  int currentChunkId = -1;
  float playerDeltaDistance = 0.0f;

  NewGameOptions worldOptions = NewGameOptions();

  std::deque<Chunk*> tempChunksToLoad;
  std::deque<Chunk*> tempChunksToUnLoad;

  u8 _updateDayNightCycle = true;

  WorldLightModel worldLightModel;

  void updateChunkByPlayerPosition(Player* player, Camera* t_camera);
  void scheduleChunksNeighbors(Chunk* t_chunk, const Vec4 currentPlayerPos,
                               u8 force_loading = 0);
  void loadScheduledChunks();
  void unloadScheduledChunks();
  void updateNeighBorsChunksByModdedPosition(const Vec4& pos);
  // void removeBlockFromChunk(Block* blockToRemove);
  void updateNeighBorsChunksByAddedBlock(Vec4* offset);
  void addChunkToLoadAsync(Chunk* t_chunk);
  void addChunkToUnloadAsync(Chunk* t_chunk);
  void updateLightModel();
  void sortChunksToLoad(const Vec4& currentPlayerPos);

  const bool getOptimalSpawnPositionInChunk(const Chunk* targetChunk,
                                            Vec4* result);
  const bool calcSpawOffsetOfChunk(Vec4* result, const Vec4& minOffset,
                                   const Vec4& maxOffset, uint16_t bias = 0);
  const bool calcSpawnOffsetByXZ(Vec4* result, const int posX, const int posZ);

  std::vector<Vec4> _targetBlockVertices;
  std::vector<Vec4> _targetBlockUVMap;
  std::vector<Color> _targetBlockColors;
  void updateTargetBlock(Camera* t_camera, Player* t_player);
  void updateBlockDamage();
  void buildTargetBlockDrawData();
  void clearTargetBlockDrawData();

  inline void setIntialTime() {
    if (worldOptions.type == WorldType::WORLD_MINI_GAME_MAZECRAFT) {
      g_ticksCounter = NIGHT_MID;
    } else {
      g_ticksCounter = static_cast<int>(worldOptions.initialTime);
    }
  };

  Ray ray;
  ItemRepository* t_itemRepository;

  // Paticles values
  float lastTimeCreatedParticle = 0.0F;

  // Breaking control
  u8 _isBreakingBlock = false;
  float breaking_time_pessed = 0.0F;
  float lastTimePlayedBreakingSfx = 0.0F;

  uint32_t seed;

  inline u8 isCrossedBlock(Blocks block_type);
  inline u32 getIndexByOffset(int x, int y, int z) {
    return (y * pLevel->map.length * pLevel->map.width) +
           (z * pLevel->map.width) + x;
  }

  /**
   * @brief Update the visible block faces
   * @return false if the block is completely hidden
   *
   */
  u8 getBlockVisibleFaces(const Vec4* t_blockOffset);
  u8 getSlabVisibleFaces(const Vec4* t_blockOffset);
  u8 getLeavesVisibleFaces(const Vec4* t_blockOffset);
  u8 getLiquidBlockVisibleFaces(const Vec4* t_blockOffset);

  inline u8 isAirAtPosition(const u8 x, const u8 y, const u8 z);
  inline u8 isLiquidAtPosition(const u8 x, const u8 y, const u8 z);

  void playPutBlockSound(const Blocks& blockType);
  void playDestroyBlockSound(const Blocks& blockType);
  void playBreakingBlockSound(const Blocks& blockType);
  void playFlowingWaterSound();

  void initWorldLightModel();

  // Liquids
  uint16_t waterSoundDuration = 4000;
  float waterSoundTimeCounter = 0.0F;
  float lastTimePlayedWaterSound = 0.0F;
  std::queue<Node> waterBfsQueue = {};
  std::queue<Node> waterRemovalBfsQueue = {};
  std::queue<Node> lavaBfsQueue = {};
  std::queue<Node> lavaRemovalBfsQueue = {};
  std::unordered_set<Chunk*> affectedChunksIdByLiquidPropagation;

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
  void updateLiquidWater();
  void propagateWaterRemovalQueue();
  void propagateWaterAddQueue();
  void updateLiquidLava();
  void propagateLavaRemovalQueue();
  void propagateLavaAddQueue();
  void updateChunksAffectedByLiquidPropagation();
  const s8 getNextLavaLevel(const s8 currentLevel);
  u8 canPropagateLiquid(uint16_t x, uint16_t y, uint16_t z);

  inline void dispatchChunkBatch() {
    unloadScheduledChunks();
    loadScheduledChunks();
  }

  bool inline isTransparent(Blocks block) {
    return block == Blocks::AIR_BLOCK || block == Blocks::WATER_BLOCK ||
           block == Blocks::GRASS || block == Blocks::POPPY_FLOWER ||
           block == Blocks::DANDELION_FLOWER || block == Blocks::TORCH ||
           block == Blocks::GLASS_BLOCK || block == Blocks::OAK_LEAVES_BLOCK ||
           block == Blocks::BIRCH_LEAVES_BLOCK ||

           // If it's slab, it's visible;
           ((u8)block >= (u8)Blocks::STONE_SLAB &&
            (u8)block <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB);
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
  void floodFillSunlightRemove(uint16_t x, uint16_t y, uint16_t z,
                               u8 lightLevel);
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
  // From CrossCraft
  std::queue<Node> lightBfsQueue;
  std::queue<Node> lightRemovalBfsQueue;

  // std::stack<Node> sunlightBfsQueue;
  std::queue<Node> sunlightBfsQueue;
  std::queue<Node> sunlightRemovalBfsQueue;

  void CrossCraft_World_Init(const uint32_t& seed);
  void CrossCraft_World_Deinit();

  /**
   * @brief Generates the world
   * @TODO Offer a callback for world percentage
   */
  void CrossCraft_World_GenerateMap(WorldType worldType);
};
