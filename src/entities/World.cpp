
#include "entities/World.hpp"
#include "entities/chunk_generator.hpp"
#include "entities/chunk_storage.hpp"
#include "managers/collision_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/tick_scheduler.hpp"
#include "debug.hpp"
#include <tyra>
#include <cmath>
#include <string>
#include "memory-monitor/memory_monitor.hpp"

# include "utils.hpp"

// From CrossCraft
#include <stdio.h>

World::World(const NewGameOptions& options, Level* level)
    : targetBlock(blockInteraction.targetBlock) {
  seed = options.seed;
  pLevel = level;
  pLevel->world = this;
  tickHandles = new TickTaskHandles();

  printf("\n\n|-----------SEED---------|");
  printf("\n%lu\n", seed);
  printf("|------------------------|\n\n");

  worldOptions = options;
  drawDistanceController.init(worldOptions.drawDistanceMode);
  _updateDayNightCycle = options.type != WorldType::WORLD_MINI_GAME_MAZECRAFT;
  setIntialTime();

  CrossCraft_World_Init(seed);
  CollisionManager_initTree();
  MeshBuilder_RegisterBuilders();

  // Initialize chunk persistence provider.
  std::string worldDir;

  if (options.fullPath.empty()) {
    worldDir = Tyra::FileUtils::fromCwd(
        std::string("saves/") + Utils::sanitizeWorldStorageName(worldOptions.name));
    worldOptions.fullPath = worldDir;
  } else {
    worldDir = options.fullPath;
  }

  chunkProvider = new ChunkProvider(
      new ChunkGenerator(pLevel, seed, worldOptions.type),
      new ChunkStorage(worldDir));
}

World::~World() {
  flushChunkProvider();
  delete tickHandles;
  tickHandles = nullptr;
  delete chunkProvider;
  chunkProvider = nullptr;
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

  // Note: onLoadedCallback for light enqueue was removed as an optimization.
  // build() already generates correct light data via buildNormaly() ->
  // MeshBuilder_BuildMesh() -> CuboidMeshBuilder_loadLightData().
  // The 250-tick periodic callback handles day/night sun drift.
};

void World::initGeneration() {
  currentGenerationPhase = GenerationPhase::Terrain;
  generationRow = 0;
  generationTotalRows = OVERWORLD_H_DISTANCE_IN_CHUNKS;
  srand(worldOptions.seed);
  TYRA_LOG("WorldGen: Starting stepped generation...");
}

bool World::generateStep() {
  auto* provider = getChunkProvider();
  if (!provider) {
    TYRA_ERROR("WorldGen: no ChunkProvider found!");
    return true;
  }

  switch (currentGenerationPhase) {
    case GenerationPhase::Terrain: {
      int cz = generationRow;
      for (int cx = 0; cx < OVERWORLD_H_DISTANCE_IN_CHUNKS; cx++) {
        provider->generateTerrain(cx, cz);
        pLevel->unloadChunk(cx, cz, false);
      }
      generationRow++;
      if (generationRow >= generationTotalRows) {
        currentGenerationPhase = GenerationPhase::Features;
        generationRow = 0;
        TYRA_LOG("WorldGen: Pass 1 (Terrain) complete.");
      }
      return false;
    }

    case GenerationPhase::Features: {
      int cz = generationRow;
      for (int cx = 0; cx < OVERWORLD_H_DISTANCE_IN_CHUNKS; cx++) {
        LevelChunk* chunk = pLevel->getChunk(cx * CHUNK_SIZE, cz * CHUNK_SIZE);
        if (chunk) {
          provider->carve(cx, cz);
        }
        pLevel->unloadChunk(cx, cz, false);
      }
      generationRow++;
      if (generationRow >= generationTotalRows) {
        currentGenerationPhase = GenerationPhase::Decoration;
        generationRow = 0;
        TYRA_LOG("WorldGen: Pass 2 (Features) complete.");
      }
      return false;
    }

    case GenerationPhase::Decoration: {
      int cz = generationRow;
      for (int cx = 0; cx < OVERWORLD_H_DISTANCE_IN_CHUNKS; cx++) {
        LevelChunk* chunk = pLevel->getChunk(cx * CHUNK_SIZE, cz * CHUNK_SIZE);
        if (chunk) {
          provider->decorate(cx, cz);
          lightPropagation.initSunLight(chunk);
          lightPropagation.initBlockLight(chunk, &blockManager);
        }
      }

      lightPropagation.updateSunlight();
      lightPropagation.updateBlockLights();

      if (cz > 0) {
        for (int x = 0; x < OVERWORLD_H_DISTANCE_IN_CHUNKS; x++) {
          pLevel->unloadChunk(x, cz - 1, false);
        }
      }

      generationRow++;
      if (generationRow >= generationTotalRows) {
        currentGenerationPhase = GenerationPhase::LightStitch;
        generationRow = 0;
        TYRA_LOG("WorldGen: Pass 2 (Decoration) complete.");
      }
      return false;
    }

    case GenerationPhase::LightStitch: {
      int cz = generationRow;
      for (int cx = 0; cx < OVERWORLD_H_DISTANCE_IN_CHUNKS; cx++) {
        pLevel->getChunk(cx * CHUNK_SIZE, cz * CHUNK_SIZE);
        if (cz < OVERWORLD_H_DISTANCE_IN_CHUNKS - 1) {
          pLevel->getChunk(cx * CHUNK_SIZE, (cz + 1) * CHUNK_SIZE);
        }
      }

      for (int cx = 0; cx < OVERWORLD_H_DISTANCE_IN_CHUNKS; cx++) {
        LevelChunk* chunk =
            pLevel->map.chunks[cx + cz * OVERWORLD_H_DISTANCE_IN_CHUNKS];
        if (chunk) {
          lightPropagation.initSunLight(chunk);
          lightPropagation.initBlockLight(chunk, &blockManager);
        }
      }

      lightPropagation.updateSunlight();
      lightPropagation.updateBlockLights();

      for (int x = 0; x < OVERWORLD_H_DISTANCE_IN_CHUNKS; x++) {
        pLevel->unloadChunk(x, cz, false);
      }

      generationRow++;
      if (generationRow >= generationTotalRows) {
        currentGenerationPhase = GenerationPhase::Finalize;
        generationRow = 0;
        TYRA_LOG("WorldGen: Pass 3 (Stitch) complete.");
      }
      return false;
    }

    case GenerationPhase::Finalize: {
      TYRA_LOG("WorldGen: Saving and finalizing map...");
      pLevel->saveAllChunks();
      pLevel->unloadAllChunks(false);
      currentGenerationPhase = GenerationPhase::Complete;
      TYRA_LOG("WorldGen: full-map generation completed.");
      return true;
    }

    default:
      return true;
  }
}

void World::generate() {
  initGeneration();
  while (!generateStep()) {
    // Sync block wait (for compatibility)
  }
}

void World::generateLight() {
#ifdef DEBUG_MODE
  size_t initialMemoryUsage = get_used_memory();
#endif

  prepareLightModelForLoading();

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

void World::prepareLightModelForLoading() {
  dayNightCycleManager.preLoad();
  updateLightModel();
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
{
  const u8 renderDist = getDrawDistanceCap(worldOptions.drawDistanceMode);
#ifdef DEBUG_MODE
  if (g_debug_menu.enableCaveCulling) {
    chunkManager.updateWithVisibilityGraph(
        t_renderer->core.renderer3D.frustumPlanes.getAll(), &t_camera->looksAt,
        t_camera->unitCirclePosition, renderDist, fixedDeltaTime);
  } else {
    chunkManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll(),
                        &t_camera->looksAt, renderDist, fixedDeltaTime);
    chunkManager.clearOccludedChunksToUnload();
  }
#else
  chunkManager.updateWithVisibilityGraph(
      t_renderer->core.renderer3D.frustumPlanes.getAll(), &t_camera->looksAt,
      t_camera->unitCirclePosition, renderDist, fixedDeltaTime);
#endif
}

  // Keep cave culling only for rendering visibility; unloading is handled
  // exclusively by directional distance in scheduleChunks().
  if (_updateDayNightCycle)
    dayNightCycleManager.update(fixedDeltaTime, &t_camera->position);
  if (liquidPropagation.hasAffectedChunks())
    liquidPropagation.updateChunksAffectedByLiquidPropagation();
  blockInteraction.updateTargetBlock(t_camera, t_player);
};

void World::update(Player* t_player, Camera* t_camera, const float deltaTime) {
  playerDeltaDistance = lastPlayerPosition.distanceTo(t_player->position);
  lastPlayerPosition.set(t_player->position);

  particlesManager.update(deltaTime, t_camera);
  mobManager.update(deltaTime);

  if (_updateDayNightCycle) {
    dayNightCycleManager.updateDuskIntensity(t_camera->looksAt - t_camera->position);
  }
};

void World::processIdleWork() {
  if (chunkProvider) {
    chunkProvider->tick();
    chunkProvider->waitIfTooManyQueuedChunks();
  }

  // Handle pending unloads first (they free memory for new loads).
  processUnloads();

  // Prune the load queue every frame to ensure out-of-range chunks are
  // removed even when the player is stationary.
  pruneLoadQueue();

  // Nothing to do if no chunk is in progress and the queue is empty.
  if (currentBuildChunk == nullptr && tempChunksToLoad.empty()) return;

  // Budget: 5 ms expressed in EE COP0 Count-register ticks
  // (Count increments at ~147.456 MHz, so 5 ms ≈ 737 280 ticks).
  static constexpr u32 IDLE_BUDGET_CYCLES = 737280u;

  processBuildQueue(IDLE_BUDGET_CYCLES);
}

void World::processUnloads() {
  if (tempChunksToUnLoad.empty()) return;
  unloadScheduledChunks();
}

void World::processBuildQueue(u32 budgetCycles) {
  u32 start;
  asm volatile("mfc0 %0, $9" : "=r"(start));

  // Process chunks until budget is exhausted or queue is empty
  while (true) {
    // ----------------------------------------------------------------
    // If no chunk is currently being built, dequeue the next one.
    // ----------------------------------------------------------------
    if (currentBuildChunk == nullptr) {
      // Early exit: queue is empty
      if (tempChunksToLoad.empty()) return;

      Chunk* chunk = tempChunksToLoad.front();
      tempChunksToLoad.pop_front();
      chunksInLoadQueue.reset(chunk->id);

      // Check memory limit before starting a new build
      // Rebuilds (Loaded state) are allowed even if over threshold to prevent stalls
      if (!drawDistanceController.canLoadMoreChunks() && !chunk->isLoaded()) {
        // Free memory by unloading scheduled chunks
        unloadScheduledChunks();
        // Re-enqueue this chunk for later
        tempChunksToLoad.push_front(chunk);
        chunksInLoadQueue.set(chunk->id);
        return;
      }

      // Skip stale entries (state changed between enqueue and dequeue)
      // Note: Loaded chunks in the queue are dirty rebuilds and must be processed.
      if (chunk->state != ChunkState::Building &&
          chunk->state != ChunkState::Loaded) {
        continue;
      }

      // Track whether this is a brand-new chunk (not already in loadedChunks)
      currentBuildIsNewChunk = !chunk->isLoaded();

#ifdef DEBUG_MODE
      chunk->buildingTimeStart = clock();
#endif

      // Initialize incremental build phases
      chunk->beginBuild();
      currentBuildChunk = chunk;
    }

    // ----------------------------------------------------------------
    // Advance the current chunk by exactly one build phase.
    // ----------------------------------------------------------------
    const bool done = currentBuildChunk->buildStep();

    if (done) {
      Chunk* chunk = currentBuildChunk;
      currentBuildChunk = nullptr;

      TCLOG("Chunk build done: %d, %d", (int)chunk->minOffset.x / CHUNK_SIZE, (int)chunk->minOffset.z / CHUNK_SIZE);
      chunk->loadedAtTick = g_ticksCounter;

      // Only add to loadedChunks when it's a genuinely new chunk
      if (currentBuildIsNewChunk && chunk->isLoaded()) {
        chunkManager.addToLoadedChunks(chunk);
      }

#ifdef DEBUG_MODE
      if (g_debug_mode) {
        chunk->timeToBuild =
            ((float)(clock() - chunk->buildingTimeStart) / CLOCKS_PER_SEC);
        printf("Time to async build chunk %i: %f\n", chunk->id,
               chunk->timeToBuild);
      }
#endif

      // Random mob spawn opportunity (1% per newly built chunk)
      if (Utils::Probability(0.01F, worldOptions.seed)) {
        Vec4 _spawnPosition;
        if (getOptimalSpawnPositionInChunk(chunk, &_spawnPosition)) {
          mobManager.spawnMobAtPosition(MobType::Pig, _spawnPosition);
        }
      }
    } else {
      // Phase 4: Yield budget if we are waiting for async data (PreLoad).
      // This prevents a single chunk from hogging the 5ms idle budget
      // doing nothing while disk I/O completes.
      if (currentBuildChunk->getBuildPhase() == BuildPhase::PreLoad) return;
    }

    // ----------------------------------------------------------------
    // Check frame budget after each phase step.
    // ----------------------------------------------------------------
    u32 now;
    asm volatile("mfc0 %0, $9" : "=r"(now));
    if ((now - start) >= budgetCycles) return;
  }
}

// Tick based logics
void World::tick() {
  particlesManager.tick();
  chunkManager.tick();
  mobManager.tick();
  cloudsManager.tick();

  if (blockInteraction.validTargetBlock() &&
      blockInteraction.targetBlock->damage > 0)
    blockInteraction.updateBlockDamage();

  t_renderer->core.setClearScreenColor(dayNightCycleManager.getSkyColor());
}

void World::setTickContext(Player* t_player, Camera* t_camera) {
  cachedPlayer = t_player;
  cachedCamera = t_camera;
}

void World::registerTickCallbacks(TickScheduler& scheduler) {
  tickHandles->add(scheduler.everyHandle(
      CLOUDS_TICKS_UPDATE, [this]() { cloudsManager.tick(); }));

  tickHandles->add(scheduler.everyHandle(DAY_NIGHT_TICKS_UPDATE, [this]() {
    if (_updateDayNightCycle) dayNightCycleManager.tick();
  }));

  // Register block interaction async light propagation continuation
  blockInteraction.registerTickCallbacks(scheduler);
  tickHandles->add(scheduler.everyHandle(250, [this]() {
    updateLightModel();
    const float currentIntensity = worldLightModel.sunLightIntensity;

    // Phase 1: Removed synchronous light propagation here.
    // It is already handled incrementally in WorldBlockInteraction::registerTickCallbacks
    // to prevent frame spikes.

    // Accumulative threshold: only update lastSunLightIntensity when we actually trigger a reload.
    // This ensures that even slow transitions eventually trigger a light update.
    if (fabsf(currentIntensity - lastSunLightIntensity) > 0.02f) {
      chunkManager.enqueueChunksToReloadLight();
      lastSunLightIntensity = currentIntensity;
    }
  }));

  tickHandles->add(scheduler.everyHandle(
      WATER_PROPAGATION_PER_TICKS,
      [this]() { liquidPropagation.updateLiquidWater(); }));

  tickHandles->add(scheduler.everyHandle(
      LAVA_PROPAGATION_PER_TICKS,
      [this]() { liquidPropagation.updateLiquidLava(); }));

  tickHandles->add(scheduler.everyHandle(
      2, [this]() { updateChunkByPlayerPosition(cachedPlayer, cachedCamera); }));

  tickHandles->add(scheduler.everyHandle(400, [this]() {
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
        }

        // Update next position by triangular distribution
        const int max_dist = OVERWORLD_MAX_DISTANCE - 1;
        int retriesLeft = 10;
        bool validPosition = false;
        while (retriesLeft > 0) {
          const int offsetX = Tyra::Math::randomi(-5, 5);
          const int offsetZ = Tyra::Math::randomi(-5, 5);
          int candidateX = randomX + offsetX;
          int candidateZ = randomZ + offsetZ;

          if (candidateX >= 0 && candidateX < max_dist &&
              candidateZ >= 0 && candidateZ < max_dist) {
            randomX = candidateX;
            randomZ = candidateZ;
            validPosition = true;
            break;
          }
          retriesLeft--;
        }

        // If no valid position found, clamp to valid range
        if (!validPosition) {
          randomX = std::max(0, std::min(randomX, max_dist - 1));
          randomZ = std::max(0, std::min(randomZ, max_dist - 1));
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
  }));

  // Register chunk manager callbacks
  chunkManager.registerTickCallbacks(scheduler);

  // Register mob manager callbacks
  mobManager.registerTickCallbacks(scheduler);
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
    // Register the built chunk in loadedChunks so it can be rendered
    if (initialChunk->isLoaded()) {
      chunkManager.addToLoadedChunks(initialChunk);
    }
    forceLoadArea(worldSpawnArea);
  }
};

void World::buildInitialPosition(const Vec4& playerPos) {
  TYRA_LOG("building initial position for loaded save at: ", playerPos.x, " ", playerPos.y, " ", playerPos.z);
  // For loaded saves: build chunks around the saved player position, not the spawn area
  Chunk* playerChunk = chunkManager.getChunkByWorldPosition(playerPos);
  if (playerChunk != nullptr) {
    playerChunk->clear();
    playerChunk->build();
    // Register the built chunk in loadedChunks so it can be rendered
    if (playerChunk->isLoaded()) {
      chunkManager.addToLoadedChunks(playerChunk);
    }
    // Force load area around saved player position
    forceLoadArea(playerPos);
  }
};

void World::resetWorldData() {
  // Cancel any in-progress incremental build before wiping chunks, otherwise
  // processIdleWork() would try to advance a build on a cleared chunk.
  if (currentBuildChunk != nullptr) {
    currentBuildChunk->cancelBuild();
    currentBuildChunk      = nullptr;
    currentBuildIsNewChunk = false;
  }

  // Drain async queues so stale entries don't re-trigger on the new world.
  tempChunksToLoad.clear();
  tempChunksToUnLoad.clear();
  chunksInLoadQueue.reset();
  chunksInUnloadQueue.reset();

  // Reset scheduling state to force initial chunk load on next update
  hasScheduledInitialChunks = false;
  lastSchedulePosition.set(0.0f, 0.0f, 0.0f);

  chunkManager.clearAllChunks();
}

void World::updateChunkByPlayerPosition(Player* t_player, Camera* t_camera) {
  Chunk* currentChunk = chunkManager.getChunkByWorldPosition(t_camera->looksAt);

  if (!currentChunk) return;

  const bool chunkChanged = t_player->currentChunkId != currentChunk->id;
  
  // Calculate distance traveled since last schedule
  const float distanceSinceLastSchedule = lastSchedulePosition.distanceTo(t_player->position);
  // Reschedule every chunk (8 blocks) of movement to keep chunks loading ahead
  const float scheduleDistanceThreshold = static_cast<float>(CHUNK_SIZE);
  const bool movedSignificantly = distanceSinceLastSchedule >= scheduleDistanceThreshold;

  // Schedule on: initial load, chunk change, or significant movement
  if (!hasScheduledInitialChunks || chunkChanged || movedSignificantly) {
    t_player->currentChunkId = currentChunk->id;
    scheduleChunks(t_player->position, t_camera->unitCirclePosition);
    lastSchedulePosition.set(t_player->position);
    hasScheduledInitialChunks = true;
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

    forceLoadArea(position);
  }
}

// Phase 2: New pipeline-based chunk scheduling with directional ellipse + hysteresis
void World::scheduleChunks(const Vec4& playerPos, const Vec4& cameraForward) {
  if (!canBuildChunk()) return;

#ifdef DEBUG_MODE
  // Phase 4: Profiling metrics for optimization validation
  static u32 totalScheduleCalls = 0;
  static u32 totalChunksProcessed = 0;
  totalScheduleCalls++;
#endif

  // Convert player position from world coordinates to block-grid coordinates
  // (block-grid is 0-127 per axis, world-coords are 16x larger)
  const Vec4 playerBlockPos(
      playerPos.x / DOUBLE_BLOCK_SIZE,
      playerPos.y / DOUBLE_BLOCK_SIZE,
      playerPos.z / DOUBLE_BLOCK_SIZE);

  // Phase 4: Prune Load Queue before adding new candidates
  pruneLoadQueue();

  // Phase 4: Re-sort Load Queue by directional distance
  // This ensures that even if we transition quickly, the most relevant
  // chunks (closest and in front) always move to the head of the queue.
  std::sort(tempChunksToLoad.begin(), tempChunksToLoad.end(),
            [this, &playerBlockPos, &cameraForward](Chunk* a, Chunk* b) {
              return drawDistanceController.getDirectionalDistanceSq(
                         a->center, playerBlockPos, cameraForward) <
                     drawDistanceController.getDirectionalDistanceSq(
                         b->center, playerBlockPos, cameraForward);
            });

  // Get base radius capped to MAX_DRAW_DISTANCE
  const u8 baseRadius = drawDistanceController.getEffectiveRadius();

  // Helper struct for candidates
  struct LoadCandidate {
    Chunk* chunk;
    float directionalDistanceSq;
  };
  struct UnloadCandidate {
    Chunk* chunk;
    float directionalDistanceSq;
  };

  static std::vector<LoadCandidate> loadCandidates;
  static std::vector<UnloadCandidate> unloadCandidates;
  loadCandidates.clear();
  unloadCandidates.clear();

  // PASSO 1: CLASSIFICAR chunks como loadable/unloadable usando histerese
  // -----------------------------------------------------------------------
  static std::vector<Chunk*> nearbyChunks;
  nearbyChunks.clear();
  // Query chunks within a slightly expanded radius to capture edge cases
  const float queryRadius = static_cast<float>(baseRadius) + 2.0f;
  chunkManager.getChunksInRadius(playerBlockPos, queryRadius, nearbyChunks);

#ifdef DEBUG_MODE
  totalChunksProcessed += nearbyChunks.size();
  if (totalScheduleCalls % 100 == 0) {
    float avgChunksPerCall =
        static_cast<float>(totalChunksProcessed) / totalScheduleCalls;
    TYRA_LOG("[Chunk Optimization] Avg chunks processed per schedule:",
             static_cast<int>(avgChunksPerCall), " / ",
             OVERWORLD_SIZE_IN_CHUNKS, " (",
             static_cast<int>((1.0f - avgChunksPerCall /
                               static_cast<float>(OVERWORLD_SIZE_IN_CHUNKS)) *
                              100.0f),
             "% reduction)");
  }
#endif

  // Bitset to track which chunks we've seen (for single-pass processing)
  static std::bitset<OVERWORLD_SIZE_IN_CHUNKS> processedChunks;
  processedChunks.reset();

  // Classify chunks from spatial grid query
  for (Chunk* t_chunk : nearbyChunks) {
    processedChunks.set(t_chunk->id);

    const float dx = playerBlockPos.x - t_chunk->center.x;
    const float dz = playerBlockPos.z - t_chunk->center.z;
    const int distance = static_cast<int>(
        sqrtf(dx * dx + dz * dz) / CHUNK_SIZE);
    t_chunk->setDistanceFromPlayerInChunks(distance);

    // Rescue chunks queued for unload that are still within range (histerese)
    if (t_chunk->isUnloading()) {
      cancelChunkUnload(t_chunk);
    }

    // Classification logic using directional distance + hysteresis
    if (!t_chunk->isBuilding() && !t_chunk->isUnloading()) {
      if (drawDistanceController.isInLoadableArea(t_chunk->center, playerBlockPos,
                                                   cameraForward)) {
        // LOADABLE: candidate for loading if not already loaded/dirty
        if (!t_chunk->isLoaded() || 
            (t_chunk->isLoaded() && t_chunk->isDirty())) {
          float dirDist = 
              drawDistanceController.getDirectionalDistanceSq(
                  t_chunk->center, playerBlockPos, cameraForward);
          loadCandidates.push_back({t_chunk, dirDist});
          // Mark as processed to prevent dual-queueing in unload pass
          processedChunks.set(t_chunk->id);
        }
      }
    }
  }

  // Classify chunks outside grid that might need unloading (farthest first)
  // Single pass: only look at loaded chunks not yet processed
  auto loadedChunks = chunkManager.getLoadedChunks();
  for (Chunk* t_chunk : *loadedChunks) {
    // Skip if already processed in grid query
    if (processedChunks.test(t_chunk->id)) continue;

    // Check unload condition (apply histerese)
    if (drawDistanceController.isInUnloadableArea(t_chunk->center, playerBlockPos,
                                                   cameraForward)) {
      t_chunk->setDistanceFromPlayerInChunks(-1);
      float dirDist = 
          drawDistanceController.getDirectionalDistanceSq(
              t_chunk->center, playerBlockPos, cameraForward);
      unloadCandidates.push_back({t_chunk, dirDist});
    }
  }

  // PASSO 2: ORDENAR candidates para otimizar carregamento/liberação
  // -----------------------------------------------------------------------
  // Load: nearest-first (prioritize closer chunks)
  std::sort(loadCandidates.begin(), loadCandidates.end(),
            [](const LoadCandidate& a, const LoadCandidate& b) {
              return a.directionalDistanceSq < b.directionalDistanceSq;
            });

  // Unload: farthest-first (free RAM from distant chunks first)
  std::sort(unloadCandidates.begin(), unloadCandidates.end(),
            [](const UnloadCandidate& a, const UnloadCandidate& b) {
              return a.directionalDistanceSq > b.directionalDistanceSq;
            });

  // PASSO 3: ENFILEIRAR chunks nas filas de load/unload
  // -----------------------------------------------------------------------
  for (const auto& candidate : loadCandidates) {
    addChunkToLoadAsync(candidate.chunk);
  }

  for (const auto& candidate : unloadCandidates) {
    addChunkToUnloadAsync(candidate.chunk);
  }

  // Phase 2: Unload data chunks that are far away from player
  const int pCX = static_cast<int>(playerPos.x / (DOUBLE_BLOCK_SIZE * CHUNK_SIZE));
  const int pCZ = static_cast<int>(playerPos.z / (DOUBLE_BLOCK_SIZE * CHUNK_SIZE));
  pLevel->unloadFarChunks(pCX, pCZ, baseRadius + 2, true);

  chunkManager.updateLoadedChunks();
}

// Force-load an area around a position (used for spawn/reload)
void World::forceLoadArea(const Vec4& centerPos) {
  if (!canBuildChunk()) return;

  // Cancel any in-progress incremental build to avoid corruption
  if (currentBuildChunk != nullptr) {
    currentBuildChunk->cancelBuild();
    currentBuildChunk = nullptr;
    currentBuildIsNewChunk = false;
  }

  // Convert center position from world coordinates to block-grid coordinates
  const Vec4 centerBlockPos(
      centerPos.x / DOUBLE_BLOCK_SIZE,
      centerPos.y / DOUBLE_BLOCK_SIZE,
      centerPos.z / DOUBLE_BLOCK_SIZE);

  const u8 baseRadius = drawDistanceController.getEffectiveRadius();
  // Note: getChunksInRadius expects radius in chunk-count, not block-count
  const float radiusInChunks = static_cast<float>(baseRadius);

  // Get all chunks within force-load radius
  static std::vector<Chunk*> forceLoadChunks;
  forceLoadChunks.clear();
  chunkManager.getChunksInRadius(centerBlockPos, radiusInChunks, forceLoadChunks);

  // Force build/rebuild synchronously for all chunks in radius
  for (Chunk* t_chunk : forceLoadChunks) {
    const float dx = centerBlockPos.x - t_chunk->center.x;
    const float dz = centerBlockPos.z - t_chunk->center.z;
    const int distance = static_cast<int>(
        sqrtf(dx * dx + dz * dz) / CHUNK_SIZE);

    if (distance <= static_cast<int>(baseRadius)) {
      // Inside force-load radius: ensure built
      // Check memory before building new chunk (graceful degradation if memory exceeded)
      if (!t_chunk->isLoaded() && !canBuildChunk()) {
        // Memory exhausted during force-load; stop preemptively
        TYRA_LOG("Warning: Memory threshold reached during forceLoadArea spawn load. " 
                 "Chunk at distance %d will load asynchronously.", distance);
        break;
      }
      
      if (t_chunk->isLoaded()) {
        t_chunk->rebuild();
      } else {
        t_chunk->build();
        // Register in loadedChunks only if newly built (not already loaded)
        chunkManager.addToLoadedChunks(t_chunk);
      }
      t_chunk->setDistanceFromPlayerInChunks(distance);
    } else {
      // Outside force-load radius: clear
      // Remove from loadedChunks before clearing if it was loaded
      if (t_chunk->isLoaded()) {
        chunkManager.removeFromLoadedChunks(t_chunk);
      }
      t_chunk->clear();
      t_chunk->setDistanceFromPlayerInChunks(-1);
    }
  }
}

void World::unloadScheduledChunks() {
  if (tempChunksToUnLoad.empty()) return;

  // Unload up to 4 chunks per frame when the queue is large to prevent
  // "zombie" chunks (loaded but beyond draw distance) from being rendered
  const int maxUnloads = (tempChunksToUnLoad.size() > 8) ? 4 : 1;
  int unloaded = 0;

  while (!tempChunksToUnLoad.empty() && unloaded < maxUnloads) {
    Chunk* chunk = tempChunksToUnLoad.front();
    tempChunksToUnLoad.pop_front();
    chunksInUnloadQueue.reset(chunk->id);

    // Validate state before clearing
    if (chunk->state != ChunkState::Unloading) {
      TYRA_LOG("Warning: chunk ", chunk->id, " not in Unloading state");
      continue;
    }

    // Remove from loaded lists before clearing (incremental O(1) removal)
    chunkManager.removeFromLoadedChunks(chunk);
    chunk->clear();
    unloaded++;
  }
}

void World::renderBlockDamageOverlay() {
  blockInteraction.renderBlockDamageOverlay();
}

void World::addChunkToLoadAsync(Chunk* t_chunk) {
  const u16 chunkId = t_chunk->id;

  // Early return: can't load if over budget
  // Early return: can't load new chunks if over budget.
  // Rebuilds (Loaded chunks) bypass this check as they are already in memory.
  if (!drawDistanceController.canLoadMoreChunks() && !t_chunk->isLoaded()) return;

  // Early return: already queued for loading
  if (chunksInLoadQueue.test(chunkId)) return;

  // Phase 2 optimization: Remove from unload queue if present (rare case)
  if (chunksInUnloadQueue.test(chunkId)) {
    for (size_t i = 0; i < tempChunksToUnLoad.size(); i++) {
      if (tempChunksToUnLoad[i]->id == chunkId) {
        tempChunksToUnLoad.erase(tempChunksToUnLoad.begin() + i);
        chunksInUnloadQueue.reset(chunkId);
        break;
      }
    }
  }

  // Mark as Building if not already loaded (dirty rebuild keeps Loaded state
  // so old geometry remains visible until the new build completes).
  if (!t_chunk->isLoaded()) {
    t_chunk->state = ChunkState::Building;
  }
  
  tempChunksToLoad.push_back(t_chunk);
  chunksInLoadQueue.set(chunkId);
}

void World::addChunkToUnloadAsync(Chunk* t_chunk) {
  const u16 chunkId = t_chunk->id;

  // Early return: already queued for unload
  if (chunksInUnloadQueue.test(chunkId)) return;

  // If this chunk is currently building incrementally, cancel it.
  if (t_chunk == currentBuildChunk) {
    currentBuildChunk->cancelBuild();
    currentBuildChunk = nullptr;
    currentBuildIsNewChunk = false;
  }

  // Protect recently-loaded chunks from immediate unload (prevents oscillation)
  // Minimum 60 ticks (~3 sec) must elapse before chunk can be unloaded
  constexpr u32 MIN_LOADED_TICKS = 60;
  if (t_chunk->loadedAtTick > 0 && 
      (g_ticksCounter - t_chunk->loadedAtTick) < MIN_LOADED_TICKS) {
    return;
  }

  // Phase 2 optimization: Remove from load queue if present (rare case)
  if (chunksInLoadQueue.test(chunkId)) {
    for (size_t i = 0; i < tempChunksToLoad.size(); i++) {
      if (tempChunksToLoad[i]->id == chunkId) {
        Chunk* removedChunk = tempChunksToLoad[i];
        
        // Clear partial data if chunk was building
        if (removedChunk->state == ChunkState::Building && !removedChunk->isLoaded()) {
          removedChunk->clearDrawDataWithoutShrink();
        }
        
        removedChunk->state = ChunkState::Clean;
        tempChunksToLoad.erase(tempChunksToLoad.begin() + i);
        chunksInLoadQueue.reset(chunkId);
        break;
      }
    }
  }

  t_chunk->state = ChunkState::Unloading;
  tempChunksToUnLoad.push_front(t_chunk);
  chunksInUnloadQueue.set(chunkId);
}

void World::cancelChunkUnload(Chunk* t_chunk) {
  const u16 chunkId = t_chunk->id;
  if (!chunksInUnloadQueue.test(chunkId)) return;

  for (size_t i = 0; i < tempChunksToUnLoad.size(); i++) {
    if (tempChunksToUnLoad[i]->id == chunkId) {
      tempChunksToUnLoad.erase(tempChunksToUnLoad.begin() + i);
      chunksInUnloadQueue.reset(chunkId);
      break;
    }
  }

  if (t_chunk->state == ChunkState::Unloading) {
    t_chunk->state = ChunkState::Loaded;
  }
}

void World::pruneLoadQueue() {
  if (tempChunksToLoad.empty() && currentBuildChunk == nullptr) return;
  if (!cachedPlayer || !cachedCamera) return;

  const Vec4 playerBlockPos(
      cachedPlayer->position.x / DOUBLE_BLOCK_SIZE,
      cachedPlayer->position.y / DOUBLE_BLOCK_SIZE,
      cachedPlayer->position.z / DOUBLE_BLOCK_SIZE);
  const Vec4& cameraForward = cachedCamera->unitCirclePosition;

  // 1. Prune candidate queue
  for (auto it = tempChunksToLoad.begin(); it != tempChunksToLoad.end();) {
    Chunk* t_chunk = *it;
    if (drawDistanceController.isInUnloadableArea(t_chunk->center,
                                                   playerBlockPos,
                                                   cameraForward)) {
      chunksInLoadQueue.reset(t_chunk->id);
      if (t_chunk->state == ChunkState::Building) {
        t_chunk->state = ChunkState::Clean;
      }
      it = tempChunksToLoad.erase(it);
    } else {
      it++;
    }
  }

  // 2. Cancel current build if it moved out of range
  if (currentBuildChunk != nullptr &&
      drawDistanceController.isInUnloadableArea(currentBuildChunk->center,
                                                 playerBlockPos,
                                                 cameraForward)) {
    currentBuildChunk->cancelBuild();
    currentBuildChunk = nullptr;
    currentBuildIsNewChunk = false;
  }
}

void World::updateLightModel() {
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.sunLightIntensity =
      dayNightCycleManager.getSunLightIntensity();
}

void World::flushChunkProvider() {
  if (!chunkProvider) return;
  chunkProvider->flush();
  chunkProvider->waitForAll();
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
  TYRA_LOG("Searching for optimal spawn position...");

  struct SpawnCandidate {
    Vec4 pos;
    int score;
  };

  SpawnCandidate bestCandidate = {
      Vec4(HALF_OVERWORLD_H_DISTANCE, HALF_OVERWORLD_V_DISTANCE,
           HALF_OVERWORLD_H_DISTANCE) *
          DOUBLE_BLOCK_SIZE,
      -1000};

  const int numCandidates = 64;
  for (int i = 0; i < numCandidates; i++) {
    const int attempt = bias + i;
    const int posX =
        static_cast<int>((seed + (attempt * 17)) % OVERWORLD_H_DISTANCE);
    const int posZ =
        static_cast<int>((seed + (attempt * 31)) % OVERWORLD_H_DISTANCE);

    int score = 0;
    int topY = -1;
    u8 topBlock = (u8)Blocks::AIR_BLOCK;

    // Find the highest non-air block
    for (int y = OVERWORLD_MAX_HEIGH - 1; y >= OVERWORLD_MIN_HEIGH; y--) {
      const u8 type = pLevel->SafeGetBlockFromMap(posX, y, posZ);
      if (type != (u8)Blocks::AIR_BLOCK && type != (u8)Blocks::VOID) {
        topY = y;
        topBlock = type;
        break;
      }
    }

    if (topY != -1) {
      // Scoring logic
      if (topBlock == (u8)Blocks::GRASS_BLOCK) score += 50;
      if (topBlock == (u8)Blocks::SAND_BLOCK) score += 30;
      if (topBlock == (u8)Blocks::WATER_BLOCK) score -= 100;
      if (topBlock == (u8)Blocks::LAVA_BLOCK) score -= 200;
      
      // Height scoring (prefer sea level)
      const int distToSeaLevel = std::abs(topY - SEA_LEVEL_Y);
      if (distToSeaLevel < 5) score += 30;
      if (topY < 16) score -= 50;
      if (topY > 80) score -= 50;

      if (score > bestCandidate.score) {
        bestCandidate.score = score;
        bestCandidate.pos = Vec4(posX, topY + 2, posZ) * DOUBLE_BLOCK_SIZE;
      }
    }

    // Unload the chunk column to keep memory usage low during search
    pLevel->unloadChunk(posX / CHUNK_SIZE, posZ / CHUNK_SIZE, false);
  }

  TYRA_LOG("Best spawn found at score: ", bestCandidate.score);
  return bestCandidate.pos;
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

void World::setDrawDistanceMode(DrawDistanceMode mode) {
  TYRA_LOG("Setting draw distance mode to ", static_cast<int>(mode));
  worldOptions.drawDistanceMode = mode;
  drawDistanceController.setMode(mode);

  // Cancel any in-progress incremental build first.
  if (currentBuildChunk != nullptr) {
    currentBuildChunk->cancelBuild();
    currentBuildChunk      = nullptr;
    currentBuildIsNewChunk = false;
  }

  // Clear async queues and properly clean up chunk states
  // Memory leak fix: Clear draw data for chunks that were building but never loaded
  for (auto chunk : tempChunksToLoad) {
    if (chunk->state == ChunkState::Building && !chunk->isLoaded()) {
      chunk->clearDrawDataWithoutShrink();
    }
    chunk->state = ChunkState::Clean;
  }
  for (auto chunk : tempChunksToUnLoad) {
    chunk->state = ChunkState::Clean;
  }
  tempChunksToLoad.clear();
  tempChunksToUnLoad.clear();

  // Clear the bitsets as well
  chunksInLoadQueue.reset();
  chunksInUnloadQueue.reset();

  // Force-reload chunks around last player position
  Chunk* currentChunk =
      chunkManager.getChunkByWorldPosition(lastPlayerPosition);
  TYRA_ASSERT(currentChunk, "Invalid chunk pointer");
  if (currentChunk) {
    forceLoadArea(lastPlayerPosition);
    delete targetBlock;
    targetBlock = nullptr;

    // Immediately schedule chunks for the new draw distance.
    // This ensures the load queue is correctly populated with the new
    // directional ellipse without waiting for the next tick.
    if (cachedPlayer && cachedCamera) {
      scheduleChunks(lastPlayerPosition, cachedCamera->unitCirclePosition);
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
      CrossCraft_WorldGenerator_Generate_Maze(pLevel);
      break;
  }
}

std::string World::getLoadQueueDebugInfo() {
  std::string info = "Queue: ";
  const size_t maxShow = 5;
  const size_t count = tempChunksToLoad.size();

  for (size_t i = 0; i < std::min(count, maxShow); ++i) {
    Chunk* c = tempChunksToLoad[i];
    info += std::to_string(c->id);
    if (c->isLoaded()) info += "R";  // Rebuild
    if (i < std::min(count, maxShow) - 1) info += ", ";
  }

  if (count > maxShow) info += "...";
  if (!drawDistanceController.canLoadMoreChunks()) {
    info += " ![MEM]";
    info += " R:" + std::to_string(drawDistanceController.getEffectiveRadius());
  }

  return info;
}

