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
#include <deque>
#include <bitset>
#include <algorithm>
#include "managers/chunk_manager.hpp"
#include "managers/clouds_manager.hpp"
#include "managers/particle/particle_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "managers/day_night_cycle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "managers/mob/mob_manager.hpp"
#include "managers/world_light_propagation.hpp"
#include "managers/world_liquid_propagation.hpp"
#include "managers/world_block_interaction.hpp"
#include "managers/draw_distance_controller.hpp"
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

class World {
 public:
  World(const NewGameOptions& options, Level* level);
  ~World();

  Level* pLevel;
  Renderer* t_renderer;

  // --- Sub-managers (public for render/tick access) ---
  MobManager mobManager;
  BlockManager blockManager;
  ChunkManager chunkManager;
  CloudsManager cloudsManager;
  ParticlesManager particlesManager;
  DayNightCycleManager dayNightCycleManager;

  // --- Extracted responsibility managers ---
  WorldLightPropagation lightPropagation;
  WorldLiquidPropagation liquidPropagation;
  WorldBlockInteraction blockInteraction;

  /**
   * Legacy public alias — references blockInteraction.targetBlock.
   * Preserves backward compatibility for external callers.
   */
  Block*& targetBlock;

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

  // --- Block interaction delegates (external API preserved) ---
  TargetedFace getTargetedFace() {
    return blockInteraction.getTargetedFace();
  }
  void placeBlockAt(const Blocks& blockType, const Vec4& blockOffset) {
    blockInteraction.placeBlockAt(blockType, blockOffset);
  }
  void removeBlock(Block* blockToRemove) {
    blockInteraction.removeBlock(blockToRemove);
  }
  void mergeSlabs(const Blocks& slabToPlace, Player* t_player,
                  const Vec4& offsetToMerge) {
    blockInteraction.mergeSlabs(slabToPlace, t_player, offsetToMerge);
  }
  bool putBlock(const Blocks& blockType, Player* t_player) {
    return blockInteraction.putBlock(blockType, t_player);
  }
  inline const u8 validTargetBlock() {
    return blockInteraction.validTargetBlock();
  };

  void setSavedSpawnArea(Vec4 pos);
  const Vec4 defineSpawnArea();
  const Vec4 calcSpawOffset(int bias = 0);
  void rebuildChunkNeighbors(Chunk* t_chunk, Vec4* moddedOffset) {
    blockInteraction.rebuildChunkNeighbors(t_chunk, moddedOffset);
  }

  inline u8 isBreakingBlock() { return blockInteraction.isBreakingBlock(); };
  void breakTargetBlock(const float& deltaTime) {
    blockInteraction.breakTargetBlock(deltaTime);
  }
  void breakTargetBlockInCreativeMode(const float& deltaTime) {
    blockInteraction.breakTargetBlockInCreativeMode(deltaTime);
  }
  void stopBreakTargetBlock() { blockInteraction.stopBreakTargetBlock(); }

  void setDrawDistanceMode(DrawDistanceMode mode);
  inline DrawDistanceMode getDrawDistanceMode() { return worldOptions.drawDistanceMode; }
  inline u8 getDrawDistance() { return drawDistanceController.getForwardDistance(); }

  void resetWorldData();
  void reloadWorldArea(const Vec4& position);

  inline size_t getVisibleChunksCount() { return chunkManager.getVisibleChunks()->size(); };
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
  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4 lastPlayerPosition;
  int currentChunkId = -1;
  float playerDeltaDistance = 0.0f;
  Vec4 lastScheduledForward = Vec4(0.0f, 0.0f, -1.0f);
  bool hasScheduledForward = false;

  NewGameOptions worldOptions = NewGameOptions();
  DrawDistanceController drawDistanceController;

  std::deque<Chunk*> tempChunksToLoad;
  std::deque<Chunk*> tempChunksToUnLoad;

  // Phase 3: Bitsets for O(1) queue deduplication (768 bytes total)
  // Chunk IDs are 0-2047, so std::bitset<2048> is perfect
  std::bitset<2048> chunksInLoadQueue;
  std::bitset<2048> chunksInUnloadQueue;

  u8 _updateDayNightCycle = true;

  WorldLightModel worldLightModel;
  float lastSunLightIntensity = 0.0f;  // Track sun intensity for delta guard

  void updateChunkByPlayerPosition(Player* player, Camera* t_camera);
  void scheduleChunksNeighbors(Chunk* t_chunk, const Vec4 currentPlayerPos,
                               const Vec4& camForward,
                               u8 force_loading = 0);
  void loadScheduledChunks();
  void unloadScheduledChunks();
  void addChunkToLoadAsync(Chunk* t_chunk);
  void addChunkToUnloadAsync(Chunk* t_chunk);
  void cancelChunkUnload(Chunk* t_chunk);
  void updateLightModel();
  void sortChunksToLoad(const Vec4& currentPlayerPos);

  const bool getOptimalSpawnPositionInChunk(const Chunk* targetChunk,
                                            Vec4* result);
  const bool calcSpawOffsetOfChunk(Vec4* result, const Vec4& minOffset,
                                   const Vec4& maxOffset, uint16_t bias = 0);
  const bool calcSpawnOffsetByXZ(Vec4* result, const int posX, const int posZ);

  inline void setIntialTime() {
    if (worldOptions.type == WorldType::WORLD_MINI_GAME_MAZECRAFT) {
      g_ticksCounter = NIGHT_MID;
    } else {
      g_ticksCounter = static_cast<int>(worldOptions.initialTime);
    }
  };

  ItemRepository* t_itemRepository;

  uint32_t seed;

  inline u32 getIndexByOffset(int x, int y, int z) {
    return (y * pLevel->map.length * pLevel->map.width) +
           (z * pLevel->map.width) + x;
  }

  inline u8 isAirAtPosition(const u8 x, const u8 y, const u8 z);
  inline u8 isLiquidAtPosition(const u8 x, const u8 y, const u8 z);

  void initWorldLightModel();

  inline void dispatchChunkBatch() {
    unloadScheduledChunks();
    loadScheduledChunks();
  }

  // FROM CrossCraft
  void CrossCraft_World_Init(const uint32_t& seed);
  void CrossCraft_World_Deinit();

  /**
   * @brief Generates the world
   * @TODO Offer a callback for world percentage
   */
  void CrossCraft_World_GenerateMap(WorldType worldType);
};
