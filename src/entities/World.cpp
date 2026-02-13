
#include "entities/World.hpp"
#include "managers/collision_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/mazecraft_generator.hpp"
#include "debug.hpp"
#include <tyra>

// From CrossCraft
#include <stdio.h>

World::World(const NewGameOptions& options, Level* level)
    : targetBlock(blockInteraction.targetBlock) {
  seed = options.seed;
  pLevel = level;

  printf("\n\n|-----------SEED---------|");
  printf("\n%lu\n", seed);
  printf("|------------------------|\n\n");

  worldOptions = options;
  _updateDayNightCycle = options.type != WorldType::WORLD_MINI_GAME_MAZECRAFT;
  setIntialTime();

  CrossCraft_World_Init(seed);
  CollisionManager_initTree();
  MeshBuilder_RegisterBuilders();
}

World::~World() {
  CrossCraft_World_Deinit();
  MeshBuilder_UnregisterBuilders();
}

void World::init(Renderer* renderer, ItemRepository* itemRepository) {
  t_renderer = renderer;

  // Init light stuff
  dayNightCycleManager.init(t_renderer);
  initWorldLightModel();

  blockManager.init(t_renderer, worldOptions.texturePack);
  chunkManager.init(&worldLightModel, pLevel);
  cloudsManager.init(t_renderer, &worldLightModel);
  particlesManager.init(t_renderer, blockManager.getBlocksTexture(),
                        worldOptions.texturePack);
  mobManager.init(t_renderer, &worldLightModel, pLevel, &chunkManager);

  // Init extracted managers
  lightPropagation.init(pLevel);
  liquidPropagation.init(pLevel, &chunkManager, &lightPropagation);
  blockInteraction.init(pLevel, t_renderer, &blockManager, &chunkManager,
                        &particlesManager, &lightPropagation,
                        &liquidPropagation, &worldLightModel);

  // Register lighting callbacks for all chunks
  auto chunks = chunkManager.getChunks();
  for (auto chunk : *chunks) {
    chunk->setOnLoadedCallback([this](Chunk* c) {
      this->chunkManager.enqueueChunkToReloadLight(c);
    });
  }
};

void World::generate() {
  if (worldOptions.type == WorldType::WORLD_MINI_GAME_MAZECRAFT) {
    unsigned int level = getWorldOptions()->seed;

    mazegen::Config cfg;
    cfg.CONSTRAIN_HALL_ONLY = false;
    cfg.EXTRA_CONNECTION_CHANCE = 0.12;

    u8 width, height;

    // setup level
    if (level < 2) {
      cfg.DEADEND_CHANCE = 0.1;
      cfg.WIGGLE_CHANCE = 0.1;

      width = 16;
      height = 16;
    } else if (level < 5) {
      cfg.DEADEND_CHANCE = 0.20;
      cfg.WIGGLE_CHANCE = 0.20;

      width = 32;
      height = 32;
    } else if (level < 10) {
      cfg.DEADEND_CHANCE = 0.3;
      cfg.WIGGLE_CHANCE = 0.3;

      width = 48;
      height = 48;
    } else if (level < 15) {
      cfg.DEADEND_CHANCE = 0.35f;
      cfg.WIGGLE_CHANCE = 0.35f;

      width = 56;
      height = 56;
    } else if (level < 20) {
      cfg.DEADEND_CHANCE = 0.4f;
      cfg.WIGGLE_CHANCE = 0.4f;

      width = 64;
      height = 64;
    } else {
      cfg.DEADEND_CHANCE = 0.5;
      cfg.WIGGLE_CHANCE = 0.5;

      width = 128;
      height = 128;
    }

    Mazecraft_GenerateMap(pLevel, level, width, height, cfg);
  } else {
    CrossCraft_World_GenerateMap(worldOptions.type);
  }
}

void World::generateLight() {
#ifdef DEBUG_MODE
  size_t initialMemoryUsage = get_used_memory();
#endif

  dayNightCycleManager.preLoad();
  updateLightModel();

  lightPropagation.initSunLight(g_ticksCounter);
  lightPropagation.initBlockLight(&blockManager);

  lightPropagation.updateSunlight();
  lightPropagation.updateBlockLights();
  chunkManager.reloadLightDataOfAllChunks();

#ifdef DEBUG_MODE
  size_t finalMemoryUsage = get_used_memory();
  float memoryUsage =
      static_cast<float>(finalMemoryUsage - initialMemoryUsage) / 1024.0f;
  printf("Memory usage for block light: %.2f KB\n", memoryUsage);
#endif
}

void World::propagateLiquids() {
  liquidPropagation.propagateAll();
}

void World::loadSpawnArea() { buildInitialPosition(); }

void World::generateSpawnArea() {
  // Define global spawn area
  worldSpawnArea.set(defineSpawnArea());
  spawnArea.set(worldSpawnArea);
  lastPlayerPosition.set(worldSpawnArea);
}

void World::setSavedSpawnArea(Vec4 pos) {
  // Define global from saved spawn area
  worldSpawnArea.set(pos);
  spawnArea.set(worldSpawnArea);
  lastPlayerPosition.set(worldSpawnArea);
}

void World::fixedUpdate(Player* t_player, Camera* t_camera,
                        const float fixedDeltaTime) {
  particlesManager.fixedUpdate(fixedDeltaTime);
  mobManager.fixedUpdate(fixedDeltaTime);

  cloudsManager.update(fixedDeltaTime);
#ifdef DEBUG_MODE
  if (g_debug_menu.enableCaveCulling) {
    chunkManager.updateWithVisibilityGraph(
        t_renderer->core.renderer3D.frustumPlanes.getAll(), &t_camera->looksAt,
        t_camera->unitCirclePosition);
  } else {
    chunkManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll(),
                        &t_camera->looksAt);
  }
#else
  chunkManager.updateWithVisibilityGraph(
      t_renderer->core.renderer3D.frustumPlanes.getAll(), &t_camera->looksAt,
      t_camera->unitCirclePosition);
#endif
  if (_updateDayNightCycle)
    dayNightCycleManager.update(fixedDeltaTime, &t_camera->position);
  if (liquidPropagation.hasAffectedChunks())
    liquidPropagation.updateChunksAffectedByLiquidPropagation();
  blockInteraction.updateTargetBlock(t_camera, t_player);
};

void World::update(Player* t_player, Camera* t_camera, const float deltaTime) {
  playerDeltaDistance = lastPlayerPosition.distanceTo(t_player->position);
  lastPlayerPosition.set(t_player->position);

  dispatchChunkBatch();

  particlesManager.update(deltaTime, t_camera);
  mobManager.update(deltaTime);
};

// Tick based logics
void World::tick(Player* t_player, Camera* t_camera) {
  particlesManager.tick();
  chunkManager.tick();
  mobManager.tick();
  cloudsManager.tick();

  if (blockInteraction.validTargetBlock() &&
      blockInteraction.targetBlock->damage > 0)
    blockInteraction.updateBlockDamage();

  if (isTicksCounterAt(CLOUDS_TICKS_UPDATE)) {
    cloudsManager.tick();
  }

  if (isTicksCounterAt(DAY_NIGHT_TICKS_UPDATE)) {
    if (_updateDayNightCycle) dayNightCycleManager.tick();
  }

  if (isTicksCounterAt(250)) {
    updateLightModel();
    lightPropagation.updateSunlight();
    lightPropagation.updateBlockLights();
    
    // Enqueue loaded chunks to reload light (uses internal validation)
    chunkManager.enqueueChunksToReloadLight();
  }

  if (isTicksCounterAt(WATER_PROPAGATION_PER_TICKS)) {
    liquidPropagation.updateLiquidWater();
  }

  if (isTicksCounterAt(LAVA_PROPAGATION_PER_TICKS)) {
    liquidPropagation.updateLiquidLava();
  }

  // Update scheduled data every 4 ticks
  if (isTicksCounterAt(5)) {
    updateChunkByPlayerPosition(t_player, t_camera);
  }

  // Try to spawn a passive mob every 20 seconds (400 ticks)
  // Based on https://minecraft.wiki/w/Mob_spawning
  if (isTicksCounterAt(400)) {
    const u8 shouldSpawnMob = Utils::Probability(0.1F);

    // Try to spawn a mob pack
    const u8 currentPassiveMobsCount =
        mobManager.getMobCountByCategory(MobCategory::Passive);
    const int mobCap = mobManager.getMobCapByCategory(MobCategory::Passive);
    const MobType randomType = static_cast<MobType>(
        Tyra::Math::randomi(0, static_cast<int>(MobType::Invalid) - 1));

    if (currentPassiveMobsCount < mobCap && shouldSpawnMob) {
      const int mobsToSpawn = 4;
      const Chunk* targetChunk = chunkManager.getLoadedChunks()->at(
          Tyra::Math::randomi(0, chunkManager.getLoadedChunks()->size() - 1));
      int randomX = Tyra::Math::randomi(targetChunk->minOffset.x,
                                        targetChunk->maxOffset.x - 1);
      int randomZ = Tyra::Math::randomi(targetChunk->minOffset.z,
                                        targetChunk->maxOffset.z - 1);
      bool spawnedPack = false;

      for (size_t i = 0; i < mobsToSpawn; i++) {
        Vec4 tempPos;
        if (calcSpawnOffsetByXZ(&tempPos, randomX, randomZ)) {
          mobManager.trySpawningMobAtPosition(MobCategory::Passive, randomType,
                                              tempPos);
          spawnedPack = true;
        } else {
          // Update next position by triangular distribution
          const int max_dist = OVERWORLD_MAX_DISTANCE - 1;
        define_triangular_distribution:
          const int offsetX = Tyra::Math::randomi(-5, 5);
          randomX += offsetX;
          const int offsetZ = Tyra::Math::randomi(-5, 5);
          randomZ += offsetZ;

          if (randomX >= max_dist || randomX < 0 || randomZ >= max_dist ||
              randomZ < 0) {
            goto define_triangular_distribution;
          }
        }
      }

      if (!spawnedPack) {
        // Try to spawn a single mob
        const int chunkIndex =
            Tyra::Math::randomi(0, chunkManager.getLoadedChunks()->size() - 1);
        Chunk* chunk = chunkManager.getLoadedChunks()->at(chunkIndex);
        Vec4 mobPos;
        if (getOptimalSpawnPositionInChunk(chunk, &mobPos)) {
          mobManager.trySpawningMobAtPosition(MobCategory::Passive, randomType,
                                              mobPos);
        }
      }
    }
  }

  t_renderer->core.setClearScreenColor(dayNightCycleManager.getSkyColor());
}

void World::renderOpaque() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderOpaque == false) return;
#endif  // DEBUG_MODE

  chunkManager.rendererOpaque(t_renderer, &blockInteraction.stapip);
};

void World::renderTransparent() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderTranslucent == false) return;
#endif  // DEBUG_MODE

  chunkManager.rendererTransparent(t_renderer, &blockInteraction.stapip);
};

void World::buildInitialPosition() {
  TYRA_LOG("building initial position...");
  Chunk* initialChunk = chunkManager.getChunkByWorldPosition(worldSpawnArea);
  if (initialChunk != nullptr) {
    initialChunk->clear();
    initialChunk->build();
    scheduleChunksNeighbors(initialChunk, lastPlayerPosition, true);
  }
};

void World::resetWorldData() { chunkManager.clearAllChunks(); }

void World::updateChunkByPlayerPosition(Player* t_player, Camera* t_camera) {
  Chunk* currentChunk = chunkManager.getChunkByWorldPosition(t_camera->looksAt);

  if (currentChunk && t_player->currentChunkId != currentChunk->id) {
    t_player->currentChunkId = currentChunk->id;
    scheduleChunksNeighbors(currentChunk, t_player->position);
  }
}

void World::reloadWorldArea(const Vec4& position) {
  Chunk* currentChunk = chunkManager.getChunkByWorldPosition(position);
  if (currentChunk) {
    if (currentChunk->isLoaded()) {
      currentChunk->rebuild();
    } else {
      currentChunk->build();
    }

    scheduleChunksNeighbors(currentChunk, position, true);
  }
}

// Phase 1: Optimized chunk scheduling using spatial grid and squared distances
void World::scheduleChunksNeighbors(Chunk* origin_chunk,
                                    const Vec4 currentPlayerPos,
                                    u8 force_loading) {
  if (!canBuildChunk()) return;

#ifdef DEBUG_MODE
  // Phase 4: Profiling metrics for optimization validation
  static u32 totalScheduleCalls = 0;
  static u32 totalChunksProcessed = 0;
  totalScheduleCalls++;
#endif

  const float maxDistance = static_cast<float>(worldOptions.drawDistance);
  const float maxDistanceSquared = maxDistance * maxDistance;

  // Vector to collect chunks needing loading/sorting
  std::vector<std::pair<Chunk*, float>> chunksToLoad;

  // Phase 1: Use spatial grid to query only chunks within radius
  // This reduces from 2048 iterations to ~100-400 (80-95% reduction)
  std::vector<Chunk*> nearbyChunks;
  chunkManager.getChunksInRadius(origin_chunk->center, maxDistance + 1.0f, nearbyChunks);

#ifdef DEBUG_MODE
  totalChunksProcessed += nearbyChunks.size();
  if (totalScheduleCalls % 100 == 0) {
    float avgChunksPerCall = static_cast<float>(totalChunksProcessed) / totalScheduleCalls;
    TYRA_LOG("[Chunk Optimization] Avg chunks processed per schedule:", 
             static_cast<int>(avgChunksPerCall), " / 2048 (",
             static_cast<int>((1.0f - avgChunksPerCall / 2048.0f) * 100.0f), "% reduction)");
  }
#endif

  // Process chunks within draw distance
  for (Chunk* t_chunk : nearbyChunks) {
    // Use squared distance to avoid sqrt (preserves ordering for sorting)
    const float distanceSquared2D = 
        ChunkManager::horizontalDistance2DSquared(origin_chunk->center, t_chunk->center) /
        (CHUNK_SIZE * CHUNK_SIZE);

    // Chunk is outside draw distance (safety check, should be rare)
    if (distanceSquared2D > maxDistanceSquared) {
      if (force_loading) {
        t_chunk->clear();
      } else if (t_chunk->isLoaded()) {
        addChunkToUnloadAsync(t_chunk);
      }
      t_chunk->setDistanceFromPlayerInChunks(-1);
      continue;
    }

    // Calculate actual distance for LOD (only when needed)
    const int distance = static_cast<int>(sqrtf(distanceSquared2D));

    if (force_loading) {
      // Synchronous mode: load/rebuild immediately
      if (t_chunk->isLoaded())
        t_chunk->rebuild();
      else
        t_chunk->build();
      t_chunk->setDistanceFromPlayerInChunks(distance);
    } else {
      // Asynchronous mode: schedule loading
      t_chunk->setDistanceFromPlayerInChunks(distance);
      
      // Only add if needs loading AND not already in a queue
      if ((t_chunk->isDirty() || !t_chunk->isLoaded()) && 
          !t_chunk->isBuilding() && !t_chunk->isUnloading()) {
        chunksToLoad.push_back(std::make_pair(t_chunk, distanceSquared2D));
      }
    }
  }

  // Unload chunks outside draw distance (check only currently loaded chunks)
  // This is much more efficient than checking all 2048 chunks
  if (!force_loading) {
    // Phase 3: Use bitset for O(1) lookup instead of O(n) search
    static std::bitset<OVERWORLD_SIZE_IN_CHUNKS> processedChunks;
    processedChunks.reset();  // Clear all bits
    
    // Mark all nearby chunks as processed using O(1) bitset set
    for (const Chunk* nearby : nearbyChunks) {
      processedChunks.set(nearby->id);
    }
    
    auto loadedChunks = chunkManager.getLoadedChunks();
    for (Chunk* t_chunk : *loadedChunks) {
      // O(1) bitset test instead of O(n) linear search
      if (processedChunks.test(t_chunk->id)) continue;

      // This chunk is loaded but outside draw distance
      addChunkToUnloadAsync(t_chunk);
      t_chunk->setDistanceFromPlayerInChunks(-1);
    }
  }

  // Sort and schedule chunks by proximity (only in async mode)
  if (!force_loading && !chunksToLoad.empty()) {
    // Sort by squared distance (avoids sqrt, preserves ordering)
    std::sort(
        chunksToLoad.begin(), chunksToLoad.end(),
        [](const std::pair<Chunk*, float>& a,
           const std::pair<Chunk*, float>& b) { return a.second < b.second; });

    // Add to load queue in sorted order
    for (const auto& pair : chunksToLoad) {
      addChunkToLoadAsync(pair.first);
    }
  }

  if (!force_loading) {
    chunkManager.updateLoadedChunks();
  }
}

void World::loadScheduledChunks() {
  if (tempChunksToLoad.empty()) return;

  Chunk* chunk = tempChunksToLoad.front();
  tempChunksToLoad.pop_front();
  chunksInLoadQueue.reset(chunk->id);  // Phase 3: Clear bitset when dequeuing
  
  // Validate state before building:
  // - New chunks should be in Building state
  // - LOD rebuilds stay in Loaded state until build() runs
  if (chunk->state != ChunkState::Building && !chunk->isLODRebuild) {
    TYRA_LOG("Warning: chunk ", chunk->id, " not in expected state for build");
    return;
  }

  chunk->build();

  if (g_debug_mode) {
    chunk->timeToBuild =
        ((float)(clock() - chunk->buildingTimeStart) / CLOCKS_PER_SEC);
    printf("Time to async build chunk %i: %f\n", chunk->id,
           chunk->timeToBuild);
  }

  if (Utils::Probability(0.01F, worldOptions.seed)) {
    Vec4 _spawnPosition;
    if (getOptimalSpawnPositionInChunk(chunk, &_spawnPosition)) {
      mobManager.spawnMobAtPosition(MobType::Pig, _spawnPosition);
    }
  }

  chunkManager.updateLoadedChunks();
}

void World::unloadScheduledChunks() {
  if (tempChunksToUnLoad.empty()) return;

  Chunk* chunk = tempChunksToUnLoad.front();
  tempChunksToUnLoad.pop_front();
  chunksInUnloadQueue.reset(chunk->id);  // Phase 3: Clear bitset when dequeuing

  // Validate state before clearing
  if (chunk->state != ChunkState::Unloading) {
    TYRA_LOG("Warning: chunk ", chunk->id, " not in Unloading state");
    return;
  }

  chunk->clear();
  chunkManager.updateLoadedChunks();
}

void World::renderBlockDamageOverlay() {
  blockInteraction.renderBlockDamageOverlay();
}

void World::addChunkToLoadAsync(Chunk* t_chunk) {
  const u16 chunkId = t_chunk->id;

  // Phase 3: O(1) duplicate check using bitset instead of O(n) linear search
  if (chunksInLoadQueue.test(chunkId)) return;

  // Remove from unload queue if present (prioritize loading)
  if (chunksInUnloadQueue.test(chunkId)) {
    // Find and remove from unload deque (rare case, so linear search acceptable)
    for (size_t i = 0; i < tempChunksToUnLoad.size(); i++) {
      if (tempChunksToUnLoad[i]->id == chunkId) {
        Chunk* removedChunk = tempChunksToUnLoad[i];
        // Clear any pending unload data to prevent memory leak
        // Only clear if chunk was in Unloading state
        if (removedChunk->state == ChunkState::Unloading) {
          removedChunk->state = ChunkState::Clean;
        }
        tempChunksToUnLoad.erase(tempChunksToUnLoad.begin() + i);
        chunksInUnloadQueue.reset(chunkId);  // Clear bitset
        break;
      }
    }
  }

  // Track if this is an LOD rebuild (already loaded, just dirty)
  t_chunk->isLODRebuild = t_chunk->isLoaded();
  
  // For LOD rebuilds, keep the Loaded state so old geometry remains visible
  // State will transition to Building only when build() actually runs
  if (!t_chunk->isLODRebuild) {
    t_chunk->state = ChunkState::Building;
  }
  
  tempChunksToLoad.push_back(t_chunk);
  chunksInLoadQueue.set(chunkId);  // Mark as queued
}

void World::addChunkToUnloadAsync(Chunk* t_chunk) {
  const u16 chunkId = t_chunk->id;

  // Phase 3: O(1) duplicate check using bitset instead of O(n) linear search
  if (chunksInUnloadQueue.test(chunkId)) return;

  // Remove from load queue if present (prioritize unloading)
  if (chunksInLoadQueue.test(chunkId)) {
    // Find and remove from load deque (rare case, so linear search acceptable)
    for (size_t i = 0; i < tempChunksToLoad.size(); i++) {
      if (tempChunksToLoad[i]->id == chunkId) {
        Chunk* removedChunk = tempChunksToLoad[i];
        
        // Clear draw data if chunk was building to prevent memory leak
        // Only clear if chunk has draw data that won't be used
        if (removedChunk->state == ChunkState::Building && !removedChunk->isLoaded()) {
          // Chunk never finished building, clear any partial data
          removedChunk->clearDrawDataWithoutShrink();
        }
        
        removedChunk->state = ChunkState::Clean;  // Reset state
        removedChunk->isLODRebuild = false;       // Reset LOD flag
        tempChunksToLoad.erase(tempChunksToLoad.begin() + i);
        chunksInLoadQueue.reset(chunkId);  // Clear bitset
        break;
      }
    }
  }

  t_chunk->state = ChunkState::Unloading;
  tempChunksToUnLoad.push_front(t_chunk);
  chunksInUnloadQueue.set(chunkId);  // Mark as queued
}

void World::updateLightModel() {
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.sunLightIntensity =
      dayNightCycleManager.getSunLightIntensity();
}

u8 World::isAirAtPosition(const u8 x, const u8 y, const u8 z) {
  if (pLevel->BoundCheckMap(x, y, z)) {
    return pLevel->GetBlockFromMap(x, y, z) == (u8)Blocks::AIR_BLOCK;
  }
  return false;
}

u8 World::isLiquidAtPosition(const u8 x, const u8 y, const u8 z) {
  if (pLevel->BoundCheckMap(x, y, z)) {
    const auto b = pLevel->GetBlockFromMap(x, y, z);
    return b == (u8)Blocks::WATER_BLOCK || b == (u8)Blocks::LAVA_BLOCK;
  }
  return false;
}

const Vec4 World::defineSpawnArea() {
  Vec4 spawPos;

  if (worldOptions.type != WorldType::WORLD_MINI_GAME_MAZECRAFT) {
    TYRA_LOG("Defining spawn area for normal world");
    spawPos = calcSpawOffset();
  } else {
    TYRA_LOG("Defining spawn area for mazecraft");
    spawPos = pLevel->offsetToWorldPos(Vec4(1.2f, 3.5f, 1.2f));
  }

  pLevel->map.spawnX = spawPos.x;
  pLevel->map.spawnY = spawPos.y;
  pLevel->map.spawnZ = spawPos.z;

  return spawPos;
}

const Vec4 World::calcSpawOffset(int bias) {
  if (bias >= CHUNK_LENGTH) {
    TYRA_LOG("Cannot find spawn position, returning default");
    return Vec4(0, 0, 0) * DOUBLE_BLOCK_SIZE;
  }

  bool found = false;
  u8 airBlockCounter = 0;
  // Pick a X and Z coordinates based on the seed;
  int posX = ((seed + bias) % HALF_OVERWORLD_H_DISTANCE);
  int posZ = ((seed - bias) % HALF_OVERWORLD_H_DISTANCE);
  Vec4 result;

  for (int posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--) {
    u8 type = pLevel->GetBlockFromMap(posX, posY, posZ);
    if (type != (u8)Blocks::AIR_BLOCK && airBlockCounter >= 4) {
      found = true;
      result = Vec4(posX, posY + 2, posZ);
      break;
    }

    if (type == (u8)Blocks::AIR_BLOCK)
      airBlockCounter++;
    else
      airBlockCounter = 0;
  }

  if (found)
    return result * DOUBLE_BLOCK_SIZE;
  else
    return calcSpawOffset(bias + 1);
}

const bool World::calcSpawnOffsetByXZ(Vec4* result, const int posX,
                                      const int posZ) {
  bool found = false;
  u8 airBlockCounter = 0;
  Vec4 offset;

  for (int posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--) {
    u8 type = pLevel->GetBlockFromMap(posX, posY, posZ);

    // TODO: implement "isSolid" at block template
    // Use it to check if the block is solid instead of checking if it's not
    // air
    if (type == (u8)Blocks::GRASS_BLOCK &&
        pLevel->GetSunLightFromMap(posX, posY + 1, posZ) == 15 &&
        airBlockCounter >= 3) {
      found = true;
      offset.set(posX, posY + 1, posZ);
      break;
    }

    if (type == (u8)Blocks::AIR_BLOCK)
      airBlockCounter++;
    else
      airBlockCounter = 0;
  }

  if (found) {
    result->set(pLevel->offsetToWorldPos(offset));
    return found;
  } else {
    return false;
  }
}

const bool World::calcSpawOffsetOfChunk(Vec4* result, const Vec4& minOffset,
                                        const Vec4& maxOffset, uint16_t bias) {
  if (bias >= CHUNK_LENGTH) {
    TYRA_LOG("Cannot find spawn position, returning default");
    return false;
  }

  bool found = false;
  u8 airBlockCounter = 0;
  // Pick a X and Z coordinates based on the min offset;
  int posX = minOffset.x + bias;
  int posZ = minOffset.z + bias;
  Vec4 tempResult;

  for (int posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--) {
    u8 type = pLevel->GetBlockFromMap(posX, posY, posZ);

    if (type == (u8)Blocks::GRASS_BLOCK &&
        pLevel->GetSunLightFromMap(posX, posY + 1, posZ) == 15 &&
        airBlockCounter >= 3) {
      found = true;
      tempResult.set(posX, posY + 1, posZ);
      break;
    }

    // TODO: implement "isSolid" at block template
    // Use it to check if the block is solid instead of checking if it's not
    // air
    if (type == (u8)Blocks::AIR_BLOCK)
      airBlockCounter++;
    else
      airBlockCounter = 0;
  }

  if (found) {
    result->set(tempResult * DOUBLE_BLOCK_SIZE);
    return found;
  } else {
    const auto hasOverZisedX = tempResult.x >= maxOffset.x;
    const auto hasOverZisedZ = tempResult.z >= maxOffset.z;

    if (hasOverZisedX && hasOverZisedZ) {
      result->set(0, 0, 0);
      return false;
    } else {
      if (hasOverZisedX) tempResult.x = minOffset.x;
      if (hasOverZisedZ) tempResult.z = minOffset.z;
    }

    return calcSpawOffsetOfChunk(result, minOffset, maxOffset, bias + 1);
  }
}

const bool World::getOptimalSpawnPositionInChunk(const Chunk* targetChunk,
                                                 Vec4* result) {
  return calcSpawOffsetOfChunk(result, targetChunk->minOffset,
                               targetChunk->maxOffset);
}

void World::setDrawDistance(const u8& drawDistanceInChunks) {
  TYRA_LOG("Setting draw distance to ", static_cast<int>(drawDistanceInChunks),
           " chunks");
  if (drawDistanceInChunks >= MIN_DRAW_DISTANCE &&
      drawDistanceInChunks <= MAX_DRAW_DISTANCE) {
    worldOptions.drawDistance = drawDistanceInChunks;
    
    // Clear async queues and properly clean up chunk states
    // Memory leak fix: Clear draw data for chunks that were building but never loaded
    for (auto chunk : tempChunksToLoad) {
      if (chunk->state == ChunkState::Building && !chunk->isLoaded()) {
        chunk->clearDrawDataWithoutShrink();
      }
      chunk->state = ChunkState::Clean;
      chunk->isLODRebuild = false;
    }
    for (auto chunk : tempChunksToUnLoad) {
      chunk->state = ChunkState::Clean;
    }
    tempChunksToLoad.clear();
    tempChunksToUnLoad.clear();
    
    // Clear the bitsets as well
    chunksInLoadQueue.reset();
    chunksInUnloadQueue.reset();
    
    Chunk* currentChunk =
        chunkManager.getChunkByWorldPosition(lastPlayerPosition);
    TYRA_ASSERT(currentChunk, "Invalid chunk pointer");
    if (currentChunk) {
      scheduleChunksNeighbors(currentChunk, lastPlayerPosition, true);
      delete targetBlock;
      targetBlock = nullptr;
    }
  }
}

void World::initWorldLightModel() {
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.sunLightIntensity =
      dayNightCycleManager.getSunLightIntensity();
}

void World::CrossCraft_World_Init(const uint32_t& seed) {
  CrossCraft_WorldGenerator_Init(rand());
}

void World::CrossCraft_World_Deinit() { TYRA_LOG("Destroying the world"); }

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void World::CrossCraft_World_GenerateMap(WorldType worldType) {
  TYRA_LOG("worldType: ", (int)worldType);

  switch (worldType) {
    case WORLD_TYPE_ORIGINAL:
      CrossCraft_WorldGenerator_Generate_Original(pLevel);
      break;
    case WORLD_TYPE_FLAT:
      CrossCraft_WorldGenerator_Generate_Flat(pLevel);
      break;
    case WORLD_TYPE_ISLAND:
      CrossCraft_WorldGenerator_Generate_Island(pLevel);
      break;
    case WORLD_TYPE_WOODS:
      CrossCraft_WorldGenerator_Generate_Woods(pLevel);
      break;
    case WORLD_TYPE_FLOATING:
      CrossCraft_WorldGenerator_Generate_Floating(pLevel);
      break;
    case WORLD_MINI_GAME_MAZECRAFT:
      break;
  }
}
