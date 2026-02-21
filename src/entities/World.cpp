
#include "entities/World.hpp"
#include "managers/collision_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/mazecraft_generator.hpp"
#include "managers/tick_scheduler.hpp"
#include "debug.hpp"
#include <tyra>
#include <cmath>

// From CrossCraft
#include <stdio.h>

World::World(const NewGameOptions& options, Level* level)
    : targetBlock(blockInteraction.targetBlock) {
  seed = options.seed;
  pLevel = level;
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
}

World::~World() {
  delete tickHandles;
  tickHandles = nullptr;
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
{
  const u8 renderDist = drawDistanceController.getForwardDistance();
#ifdef DEBUG_MODE
  if (g_debug_menu.enableCaveCulling) {
    chunkManager.updateWithVisibilityGraph(
        t_renderer->core.renderer3D.frustumPlanes.getAll(), &t_camera->looksAt,
        t_camera->unitCirclePosition, renderDist);
  } else {
    chunkManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll(),
                        &t_camera->looksAt, renderDist);
    chunkManager.clearOccludedChunksToUnload();
  }
#else
  chunkManager.updateWithVisibilityGraph(
      t_renderer->core.renderer3D.frustumPlanes.getAll(), &t_camera->looksAt,
      t_camera->unitCirclePosition, renderDist);
#endif
}

  // Occluded chunk unload (with recently-loaded protection applied in addChunkToUnloadAsync)
  auto occludedChunks = chunkManager.getOccludedChunksToUnload();
  for (Chunk* chunk : *occludedChunks) {
    addChunkToUnloadAsync(chunk);
  }
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
};

void World::processIdleWork() {
  // Handle pending unloads first (they free memory for new loads)
  if (!tempChunksToUnLoad.empty()) {
    unloadScheduledChunks();
  }

  if (tempChunksToLoad.empty()) return;

  // Budget: 5ms = 5 * 147456 EE count-register cycles
  static constexpr u32 IDLE_BUDGET_CYCLES = 737280u;

  u32 start;
  asm volatile("mfc0 %0, $9" : "=r"(start));

  bool anyBuilt = false;

  while (!tempChunksToLoad.empty()) {
    Chunk* chunk = tempChunksToLoad.front();
    tempChunksToLoad.pop_front();
    chunksInLoadQueue.reset(chunk->id);

    if (!drawDistanceController.canLoadMoreChunks()) {
      unloadScheduledChunks();
      tempChunksToLoad.push_front(chunk);
      chunksInLoadQueue.set(chunk->id);
      break;
    }

    if (chunk->state != ChunkState::Building && !chunk->isLODRebuild) {
      continue;
    }

    const bool wasLoaded = chunk->isLoaded();
    chunk->build();
    chunk->loadedAtTick = g_ticksCounter;

    // Incremental list update: only add newly loaded chunks
    if (!wasLoaded && chunk->isLoaded()) {
      chunkManager.addToLoadedChunks(chunk);
    }

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

    u32 now;
    asm volatile("mfc0 %0, $9" : "=r"(now));
    if ((now - start) >= IDLE_BUDGET_CYCLES) break;
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

  tickHandles->add(scheduler.everyHandle(250, [this]() {
    updateLightModel();
    const float prev = lastSunLightIntensity;
    lastSunLightIntensity = worldLightModel.sunLightIntensity;
    lightPropagation.updateSunlight();
    lightPropagation.updateBlockLights();
    if (fabsf(lastSunLightIntensity - prev) > 0.01f)
      chunkManager.enqueueChunksToReloadLight();
  }));

  tickHandles->add(scheduler.everyHandle(
      WATER_PROPAGATION_PER_TICKS,
      [this]() { liquidPropagation.updateLiquidWater(); }));

  tickHandles->add(scheduler.everyHandle(
      LAVA_PROPAGATION_PER_TICKS,
      [this]() { liquidPropagation.updateLiquidLava(); }));

  tickHandles->add(scheduler.everyHandle(
      5, [this]() { updateChunkByPlayerPosition(cachedPlayer, cachedCamera); }));

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
    scheduleChunksNeighbors(initialChunk, lastPlayerPosition,
                            drawDistanceController.getSmoothedForward(), true);
  }
};

void World::resetWorldData() { chunkManager.clearAllChunks(); }

void World::updateChunkByPlayerPosition(Player* t_player, Camera* t_camera) {
  Chunk* currentChunk = chunkManager.getChunkByWorldPosition(t_camera->looksAt);

  if (!currentChunk) return;

  drawDistanceController.update(t_camera->unitCirclePosition);
  const Vec4& smoothedForward = drawDistanceController.getSmoothedForward();

  const bool chunkChanged = t_player->currentChunkId != currentChunk->id;
  const float forwardDot =
      (smoothedForward.x * lastScheduledForward.x) +
      (smoothedForward.z * lastScheduledForward.z);
  // Increased threshold from 0.96 to 0.90 (~16° to ~26° rotation required)
  // to reduce excessive rescheduling from minor camera movement
  const bool forwardChanged = !hasScheduledForward || forwardDot < 0.90f;

  if (chunkChanged || forwardChanged) {
    t_player->currentChunkId = currentChunk->id;
    scheduleChunksNeighbors(currentChunk, t_player->position, smoothedForward);
    lastScheduledForward = smoothedForward;
    hasScheduledForward = true;
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

    scheduleChunksNeighbors(currentChunk, position,
                            drawDistanceController.getSmoothedForward(), true);
  }
}

// Phase 1: Optimized chunk scheduling using spatial grid and squared distances
void World::scheduleChunksNeighbors(Chunk* origin_chunk,
                                    const Vec4 currentPlayerPos,
                                    const Vec4& camForward,
                                    u8 force_loading) {
  if (!canBuildChunk()) return;

#ifdef DEBUG_MODE
  // Phase 4: Profiling metrics for optimization validation
  static u32 totalScheduleCalls = 0;
  static u32 totalChunksProcessed = 0;
  totalScheduleCalls++;
#endif

  // NOTE: drawDistanceController.update() removed from here to prevent double-update
  // It's already called in updateChunkByPlayerPosition() before scheduling is triggered

  const float forwardDistance =
      static_cast<float>(drawDistanceController.getForwardDistance());
  const float backwardDistance =
      static_cast<float>(drawDistanceController.getBackwardDistance());
  const float sideDistance = drawDistanceController.getSideDistance();

  // Circular unload zone (non-directional) prevents oscillation on rotation
  const float unloadDistance =
      static_cast<float>(drawDistanceController.getUnloadDistance());
  const float unloadDistanceSquared = unloadDistance * unloadDistance;

  // Query radius must cover the full unload zone
  const float maxQueryDistance = unloadDistance;

  // Elliptical load zone (forward-biased) for memory-efficient new loads
  const float forwardDistanceSquared = forwardDistance * forwardDistance;
  const float backwardDistanceSquared = backwardDistance * backwardDistance;
  const float sideDistanceSquared = sideDistance * sideDistance;

  const Vec4& forwardDir = drawDistanceController.getSmoothedForward();
  const int playerChunkY = static_cast<int>(
      std::floor((currentPlayerPos.y / DOUBLE_BLOCK_SIZE) / CHUNK_SIZE));

  // Vector to collect chunks needing loading/sorting
  struct LoadCandidate {
    Chunk* chunk;
    float distanceSquared;
    float forwardDot;
  };
  // Phase 6 optimization: static vectors avoid heap allocation every 250ms
  static std::vector<LoadCandidate> chunksToLoad;
  chunksToLoad.clear();

  // Phase 1: Use spatial grid to query only chunks within radius
  // This reduces from 2048 iterations to ~100-400 (80-95% reduction)
  static std::vector<Chunk*> nearbyChunks;
  nearbyChunks.clear();
  chunkManager.getChunksInRadius(origin_chunk->center,
                                 maxQueryDistance + 1.0f, nearbyChunks);

#ifdef DEBUG_MODE
  totalChunksProcessed += nearbyChunks.size();
  if (totalScheduleCalls % 100 == 0) {
    float avgChunksPerCall =
        static_cast<float>(totalChunksProcessed) / totalScheduleCalls;
    TYRA_LOG("[Chunk Optimization] Avg chunks processed per schedule:",
             static_cast<int>(avgChunksPerCall), " / 2048 (",
             static_cast<int>((1.0f - avgChunksPerCall / 2048.0f) * 100.0f),
             "% reduction)");
  }
#endif

  // Phase 3: Use bitset for O(1) lookup instead of O(n) search
  static std::bitset<OVERWORLD_SIZE_IN_CHUNKS> processedChunks;
  processedChunks.reset();

  for (Chunk* t_chunk : nearbyChunks) {
    const int chunkX = static_cast<int>(t_chunk->minOffset.x / CHUNK_SIZE);
    const int chunkZ = static_cast<int>(t_chunk->minOffset.z / CHUNK_SIZE);
    const int chunkY = static_cast<int>(t_chunk->minOffset.y / CHUNK_SIZE);

    u8 topChunkY = 0;
    bool columnHasBlocks = false;
    chunkManager.getColumnHeightInfo(chunkX, chunkZ, topChunkY, columnHasBlocks);

    if (!columnHasBlocks) {
      if (force_loading) {
        t_chunk->clear();
      } else if (t_chunk->isLoaded()) {
        addChunkToUnloadAsync(t_chunk);
      }
      t_chunk->setDistanceFromPlayerInChunks(-1);
      continue;
    }

    if (chunkY > static_cast<int>(topChunkY)) {
      if (force_loading) {
        t_chunk->clear();
      } else if (t_chunk->isLoaded()) {
        addChunkToUnloadAsync(t_chunk);
      }
      t_chunk->setDistanceFromPlayerInChunks(-1);
      continue;
    }

    if (chunkY + 2 < static_cast<int>(topChunkY)) {
      if (std::abs(playerChunkY - chunkY) > 2) {
        if (force_loading) {
          t_chunk->clear();
        } else if (t_chunk->isLoaded()) {
          addChunkToUnloadAsync(t_chunk);
        }
        t_chunk->setDistanceFromPlayerInChunks(-1);
        continue;
      }
    }

    const float dx = (t_chunk->center.x - origin_chunk->center.x) / CHUNK_SIZE;
    const float dz = (t_chunk->center.z - origin_chunk->center.z) / CHUNK_SIZE;
    const float distanceSquared2D = dx * dx + dz * dz;
    if (distanceSquared2D <= 0.0001f) {
      processedChunks.set(t_chunk->id);
    }

    // Circular unload check (non-directional) — prevents oscillation on rotation
    // Chunks beyond the unload radius are unloaded regardless of camera direction
    if (distanceSquared2D > unloadDistanceSquared) {
      if (force_loading) {
        t_chunk->clear();
      } else if (t_chunk->isLoaded()) {
        addChunkToUnloadAsync(t_chunk);
      }
      t_chunk->setDistanceFromPlayerInChunks(-1);
      continue;
    }

    processedChunks.set(t_chunk->id);

    const int distance = static_cast<int>(sqrtf(distanceSquared2D));

    // Elliptical load zone (forward-biased) for memory-efficient new loads
    const float forwardDot = (dx * forwardDir.x) + (dz * forwardDir.z);
    const float forwardAbs = fabsf(forwardDot);
    const float forwardDistanceLimitSquared =
        (forwardDot >= 0.0f) ? forwardDistanceSquared : backwardDistanceSquared;
    const float perpSquared =
        std::max(0.0f, distanceSquared2D - (forwardDot * forwardDot));
    const float ellipseValue =
        (forwardAbs * forwardAbs) / forwardDistanceLimitSquared +
        (perpSquared / sideDistanceSquared);
    const bool insideLoadZone = (ellipseValue <= 1.0f);

    if (force_loading) {
      if (insideLoadZone) {
        if (t_chunk->isLoaded())
          t_chunk->rebuild();
        else
          t_chunk->build();
        t_chunk->setDistanceFromPlayerInChunks(distance);
      } else {
        t_chunk->clear();
        t_chunk->setDistanceFromPlayerInChunks(-1);
      }
    } else {
      // Rescue chunks queued for unload that are still within range
      if (t_chunk->isUnloading()) {
        cancelChunkUnload(t_chunk);
      }

      t_chunk->setDistanceFromPlayerInChunks(distance);

      // Only queue NEW loads within the elliptical load zone
      // Chunks between ellipse and unload circle: keep current state
      if (insideLoadZone &&
          (t_chunk->isDirty() || !t_chunk->isLoaded()) &&
          !t_chunk->isBuilding() && !t_chunk->isUnloading()) {
        chunksToLoad.push_back({t_chunk, distanceSquared2D, forwardDot});
      }
    }
  }

  if (!force_loading) {
    // Phase 6 optimization: static vector avoids heap allocation every call
    static std::vector<std::pair<Chunk*, float>> unloadCandidates;
    unloadCandidates.clear();
    auto loadedChunks = chunkManager.getLoadedChunks();
    unloadCandidates.reserve(loadedChunks->size());

    for (Chunk* t_chunk : *loadedChunks) {
      if (processedChunks.test(t_chunk->id)) continue;

      const float dx = (t_chunk->center.x - origin_chunk->center.x) / CHUNK_SIZE;
      const float dz = (t_chunk->center.z - origin_chunk->center.z) / CHUNK_SIZE;
      const float dot = (dx * forwardDir.x) + (dz * forwardDir.z);
      unloadCandidates.push_back(std::make_pair(t_chunk, dot));
      t_chunk->setDistanceFromPlayerInChunks(-1);
    }

    std::sort(unloadCandidates.begin(), unloadCandidates.end(),
              [](const std::pair<Chunk*, float>& a,
                 const std::pair<Chunk*, float>& b) {
                return a.second < b.second;
              });

    for (auto it = unloadCandidates.rbegin(); it != unloadCandidates.rend();
         ++it) {
      addChunkToUnloadAsync(it->first);
    }
  }

  if (!force_loading && !chunksToLoad.empty()) {
    // Fixed: Use stable sort by distance (primary) and forward dot (secondary with epsilon)
    // Previous float equality comparison caused unstable ordering
    std::sort(chunksToLoad.begin(), chunksToLoad.end(),
              [](const LoadCandidate& a, const LoadCandidate& b) {
                // Primary: distance (closer chunks first)
                if (fabsf(a.distanceSquared - b.distanceSquared) > 0.1f)
                  return a.distanceSquared < b.distanceSquared;
                // Secondary: forward alignment (more aligned first)
                return a.forwardDot > b.forwardDot;
              });

    for (const auto& candidate : chunksToLoad) {
      addChunkToLoadAsync(candidate.chunk);
    }
  }

  if (!force_loading) {
    chunkManager.updateLoadedChunks();
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

  if (!drawDistanceController.canLoadMoreChunks()) return;

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
  
  // Protect recently-loaded chunks from immediate unload (prevents oscillation)
  // Minimum 60 ticks (~3 sec) must elapse before chunk can be unloaded
  constexpr u32 MIN_LOADED_TICKS = 60;
  if (t_chunk->loadedAtTick > 0 && 
      (g_ticksCounter - t_chunk->loadedAtTick) < MIN_LOADED_TICKS) {
    return;
  }

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

void World::setDrawDistanceMode(DrawDistanceMode mode) {
  TYRA_LOG("Setting draw distance mode to ", static_cast<int>(mode));
  worldOptions.drawDistanceMode = mode;
  drawDistanceController.setMode(mode);

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
    scheduleChunksNeighbors(currentChunk, lastPlayerPosition,
                            drawDistanceController.getSmoothedForward(), true);
    delete targetBlock;
    targetBlock = nullptr;
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
