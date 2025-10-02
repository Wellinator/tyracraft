
#include "entities/World.hpp"
#include "entities/entity.hpp"
#include "3libs/bvh/bvh.h"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "managers/model_builder.hpp"
#include "managers/collision_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/mazecraft_generator.hpp"
#include "managers/visible_faces_manager.hpp"
#include "managers/light_manager.hpp"
#include "managers/clipping_manager.hpp"
#include "debug.hpp"
#include <tyra>

// From CrossCraft
#include <stdio.h>
#include "entities/World.hpp"
#include <queue>
#include <stack>

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
using Tyra::Color;
using Tyra::M4x4;

World::World(const NewGameOptions& options, Level* level) {
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
  affectedChunksIdByLiquidPropagation.clear();

  clearTargetBlockDrawData();
  CrossCraft_World_Deinit();
  MeshBuilder_UnregisterBuilders();
}

void World::init(Renderer* renderer, ItemRepository* itemRepository) {
  // Set renders
  t_renderer = renderer;
  stapip.setRenderer(&t_renderer->core);

  // Set soundManager ref

  // Init light stuff
  dayNightCycleManager.init(t_renderer);
  initWorldLightModel();

  blockManager.init(t_renderer, worldOptions.texturePack);
  chunkManager.init(&worldLightModel, pLevel);
  cloudsManager.init(t_renderer, &worldLightModel);
  particlesManager.init(t_renderer, blockManager.getBlocksTexture(),
                        worldOptions.texturePack);
  mobManager.init(t_renderer, &worldLightModel, pLevel, &chunkManager);
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
#endif  // end if DEBUG_MODE

  dayNightCycleManager.preLoad();
  updateLightModel();

  initSunLight(g_ticksCounter);
  initBlockLight(&blockManager);

  updateSunlight();
  updateBlockLights();
  chunkManager.reloadLightDataOfAllChunks();

#ifdef DEBUG_MODE
  size_t finalMemoryUsage = get_used_memory();
  float memoryUsage =
      static_cast<float>(finalMemoryUsage - initialMemoryUsage) / 1024.0f;
  printf("Memory usage for block light: %.2f KB\n", memoryUsage);
#endif  // end if DEBUG_MODE
}

void World::propagateLiquids() {
  TYRA_LOG("Propagating liquids...");
  initLiquidExpansion();

  while (waterBfsQueue.empty() == false) propagateWaterAddQueue();
  while (lavaBfsQueue.empty() == false) propagateLavaAddQueue();
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
  chunkManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll(),
                      &t_camera->looksAt);
  if (_updateDayNightCycle)
    dayNightCycleManager.update(fixedDeltaTime, &t_camera->position);
  if (affectedChunksIdByLiquidPropagation.size() > 0)
    updateChunksAffectedByLiquidPropagation();
  updateTargetBlock(t_camera, t_player);
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

  if (validTargetBlock() && targetBlock->damage > 0) updateBlockDamage();

  if (isTicksCounterAt(CLOUDS_TICKS_UPDATE)) {
    cloudsManager.tick();
  }

  if (isTicksCounterAt(DAY_NIGHT_TICKS_UPDATE)) {
    if (_updateDayNightCycle) dayNightCycleManager.tick();
  }

  if (isTicksCounterAt(250)) {
    updateLightModel();
    updateSunlight();
    updateBlockLights();
  }

  if (isTicksCounterAt(WATER_PROPAGATION_PER_TICKS)) {
    updateLiquidWater();
  }

  if (isTicksCounterAt(LAVA_PROPAGATION_PER_TICKS)) {
    updateLiquidLava();
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

  chunkManager.rendererOpaque(t_renderer, &stapip);
};

void World::renderTransparent() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderTranslucent == false) return;
#endif  // DEBUG_MODE

  chunkManager.rendererTransparent(t_renderer, &stapip);
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

void World::updateNeighBorsChunksByAddedBlock(Vec4* offset) {
  Chunk* currentChunk = chunkManager.getChunkByBlockOffset(*offset);
  rebuildChunkNeighbors(currentChunk, offset);
}

// TODO: refactor to use BFS algorithm instead loop
void World::scheduleChunksNeighbors(Chunk* origin_chunk,
                                    const Vec4 currentPlayerPos,
                                    u8 force_loading) {
  if (!canBuildChunk()) return;

  auto chunks = chunkManager.getChunks();
  for (u16 i = 0; i < chunks->size(); i++) {
    auto t_chunk = (*chunks)[i];
    const int distance =
        origin_chunk->center.distanceTo(t_chunk->center) / CHUNK_SIZE;

    if (distance > worldOptions.drawDistance) {
      if (force_loading) {
        t_chunk->clear();
      } else if (t_chunk->isLoaded()) {
        addChunkToUnloadAsync(t_chunk);
      }
      t_chunk->setDistanceFromPlayerInChunks(-1);
    } else {
      if (force_loading) {
        if (t_chunk->isLoaded())
          t_chunk->rebuild();
        else
          t_chunk->build();
      } else if (t_chunk->state == ChunkState::Clean) {
        addChunkToLoadAsync(t_chunk);
      } else {
        if (t_chunk->getLODFromDistance(distance) !=
            t_chunk->getLODFromDistance()) {
          t_chunk->setDistanceFromPlayerInChunks(distance);
          t_chunk->onLodChanged();
        }
      }

      t_chunk->setDistanceFromPlayerInChunks(distance);
    }
  }

  chunkManager.updateLoadedChunks();
  if (!force_loading && !tempChunksToLoad.empty())
    sortChunksToLoad(currentPlayerPos);
}

void World::sortChunksToLoad(const Vec4& currentPlayerPos) {
  std::sort(tempChunksToLoad.begin(), tempChunksToLoad.end(),
            [currentPlayerPos](const Chunk* a, const Chunk* b) {
              auto distanceA =
                  ((a->center * DOUBLE_BLOCK_SIZE) - currentPlayerPos).length();
              auto distanceB =
                  ((b->center * DOUBLE_BLOCK_SIZE) - currentPlayerPos).length();

              return distanceA < distanceB;
            });
}

void World::loadScheduledChunks() {
  if (tempChunksToLoad.size() > 0) {
    Chunk* chunk = tempChunksToLoad.front();
    chunk->build();

    if (g_debug_mode) {
      chunk->timeToBuild =
          ((float)(clock() - chunk->buildingTimeStart) / CLOCKS_PER_SEC);
      printf("Time to async build chunk %i: %f\n", chunk->id,
             chunk->timeToBuild);
    }

    if (Utils::Probability(0.1F, worldOptions.seed)) {
      Vec4 _spawnPosition;
      if (getOptimalSpawnPositionInChunk(chunk, &_spawnPosition)) {
        mobManager.spawnMobAtPosition(MobType::Pig, _spawnPosition);
      }
    }

    tempChunksToLoad.pop_front();
    return;
  }

  if (tempChunksToLoad.size() == 0) {
    tempChunksToLoad.clear();
    tempChunksToLoad.shrink_to_fit();
    chunkManager.updateLoadedChunks();
  }
}

void World::unloadScheduledChunks() {
  if (tempChunksToUnLoad.size() > 0) {
    Chunk* chunk = tempChunksToUnLoad.front();
    if (chunk->state != ChunkState::Clean) {
      chunk->clear();
      tempChunksToUnLoad.pop_front();
    }
    return;
  }

  if (tempChunksToUnLoad.size() == 0) {
    tempChunksToUnLoad.clear();
    tempChunksToUnLoad.shrink_to_fit();
  }
  chunkManager.updateLoadedChunks();
}

void World::renderBlockDamageOverlay() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderBlockDamage == false) return;
#endif  // DEBUG_MODE

  if (!targetBlock) return;

  t_renderer->renderer3D.usePipeline(stapip);

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;
  M4x4 model = M4x4::Identity;

  // std::vector<Vec4> outVertices;
  // std::vector<Vec4> outUVMap;
  // std::vector<Color> outColors;

  // outVertices.reserve(_targetBlockVertices.size());
  // outUVMap.reserve(_targetBlockVertices.size());
  // outColors.reserve(_targetBlockVertices.size());

  // ClippingManager_ClipMesh(_targetBlockVertices, _targetBlockUVMap,
  //                          _targetBlockColors, outVertices, outUVMap,
  //                          outColors, t_renderer,
  //                          Camera::getInstance()->looksAt);

  infoBag.model = &model;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  colorBag.many = _targetBlockColors.data();
  bag.color = &colorBag;

  textureBag.coordinates = _targetBlockUVMap.data();
  textureBag.texture = blockManager.getBlocksTexture();
  bag.texture = &textureBag;

  bag.count = _targetBlockVertices.size();
  bag.vertices = _targetBlockVertices.data();
  bag.info = &infoBag;

  ClippingManager_ClipAndRenderBag(&bag, &stapip, t_renderer,
                                   Camera::getInstance()->looksAt);

#ifdef DEBUG_MODE
  if (g_debug_menu.showTargetedBlockBoundingBox) {
    BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(&targetBlock->offset);
    M4x4 model = ModelBuilder_BuildModel(&targetBlock->offset);
    BBox blockBBox = rawBBox->getTransformed(model);
    t_renderer->renderer3D.utility.drawBBox(blockBBox, Color(100, 100, 50));
  }
#endif  // DEBUG_MODE
}

void World::addChunkToLoadAsync(Chunk* t_chunk) {
  // Avoid being duplicated;
  for (size_t i = 0; i < tempChunksToLoad.size(); i++)
    if (tempChunksToLoad[i]->id == t_chunk->id) return;

  // // Avoid unload and load the same chunk at the same time
  // for (size_t i = 0; i < tempChunksToUnLoad.size(); i++)
  //   if (tempChunksToUnLoad[i]->id == t_chunk->id) return;

  tempChunksToLoad.push_front(t_chunk);
}

void World::addChunkToUnloadAsync(Chunk* t_chunk) {
  // Avoid being duplicated;
  for (size_t i = 0; i < tempChunksToUnLoad.size(); i++)
    if (tempChunksToUnLoad[i]->id == t_chunk->id) return;

  // // Avoid unload and load the same chunk at the same time
  // for (size_t i = 0; i < tempChunksToLoad.size(); i++)
  //   if (tempChunksToLoad[i]->id == t_chunk->id)
  //     tempChunksToLoad.erase(tempChunksToLoad.begin() + i);

  tempChunksToUnLoad.push_front(t_chunk);
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

TargetedFace World::getTargetedFace() {
  Vec4 targetPos = ray.at(targetBlock->distance);

  float frontFace = targetBlock->bbox->getFrontFace().axisPosition;
  if (std::round(targetPos.z) == frontFace) {
    return TargetedFace::FrontFace;
  }

  float backFace = targetBlock->bbox->getBackFace().axisPosition;
  if (std::round(targetPos.z) == backFace) {
    return TargetedFace::BackFace;
  }

  float leftFace = targetBlock->bbox->getLeftFace().axisPosition;
  if (std::round(targetPos.x) == leftFace) {
    return TargetedFace::LeftFace;
  }

  float rightFace = targetBlock->bbox->getRightFace().axisPosition;
  if (std::round(targetPos.x) == rightFace) {
    return TargetedFace::RightFace;
  }

  float topFace = targetBlock->bbox->getTopFace().axisPosition;
  if (std::round(targetPos.y) == topFace) {
    return TargetedFace::TopFace;
  }

  float bottomFace = targetBlock->bbox->getBottomFace().axisPosition;
  if (std::round(targetPos.y) == bottomFace) {
    return TargetedFace::BottomFace;
  }

  if (g_debug_mode) {
    TYRA_ERROR("Could not determine targeted face!");
    TYRA_ERROR("------ Target bbox faces -----");
    targetPos.print("Target Position: ");
    printf("Front Face: %f\n", frontFace);
    printf("Back Face: %f\n", backFace);
    printf("Left Face: %f\n", leftFace);
    printf("Right Face: %f\n", rightFace);
    printf("Top Face: %f\n", topFace);
    printf("Bottom Face: %f\n", bottomFace);
    TYRA_ERROR("------------------------------");
  }

  return TargetedFace::TopFace;  // Default
}

void World::placeBlockAt(const Blocks& blockType, const Vec4& blockOffset) {
  const Blocks blockTypeAtOffsetPosition = static_cast<Blocks>(
      pLevel->GetBlockFromMap(blockOffset.x, blockOffset.y, blockOffset.z));

  BlockOrientation orientation = BlockOrientation::East;

  if (blockManager.isBlockOriented(blockType)) {
    const float cameraYaw = Camera::getInstance()->yaw;

    if (cameraYaw > 315 || cameraYaw < 45) {
      orientation = BlockOrientation::North;
    } else if (cameraYaw >= 135 && cameraYaw <= 225) {
      orientation = BlockOrientation::South;
    } else if (cameraYaw >= 45 && cameraYaw <= 135) {
      orientation = BlockOrientation::East;
    } else {
      orientation = BlockOrientation::West;
    }
  }
  pLevel->SetBlockOrientationDataToMap(blockOffset.x, blockOffset.y,
                                       blockOffset.z, orientation);

  pLevel->SetBlockInMap(blockOffset.x, blockOffset.y, blockOffset.z,
                        static_cast<u8>(blockType));
  checkSunLightAt(blockOffset.x, blockOffset.y, blockOffset.z);

  const auto lightValue = blockManager.getBlockLightValue(blockType);
  if (lightValue > 0) {
    addBlockLight(blockOffset.x, blockOffset.y, blockOffset.z, lightValue);
  } else {
    removeLight(blockOffset.x, blockOffset.y, blockOffset.z);
  }

  updateSunlight();
  updateBlockLights();

  chunkManager.reloadLightData();

  const Blocks oldTypeBlock = blockTypeAtOffsetPosition;
  const u8 isPlacingLiquid =
      blockType == Blocks::WATER_BLOCK || blockType == Blocks::LAVA_BLOCK;
  const u8 itWasLiquidAtPosition =
      oldTypeBlock == Blocks::WATER_BLOCK || oldTypeBlock == Blocks::LAVA_BLOCK;

  if (isPlacingLiquid) {
    addLiquid(blockOffset.x, blockOffset.y, blockOffset.z, (u8)blockType,
              (u8)LiquidLevel::Percent100, (u8)orientation);
  } else if (itWasLiquidAtPosition) {
    removeLiquid(blockOffset.x, blockOffset.y, blockOffset.z, (u8)oldTypeBlock);
  }

  updateNeighBorsChunksByAddedBlock(const_cast<Vec4*>(&blockOffset));
}

void World::removeBlock(Block* blockToRemove) {
  // Generate amount of particles right begore block gets destroyed
  particlesManager.createBlockParticleBatch(blockToRemove, 48);

  Vec4 offsetToRemove = blockToRemove->offset;
  pLevel->SetBlockInMapByIndex(blockToRemove->index, (u8)Blocks::AIR_BLOCK);
  pLevel->SetLiquidDataToMap(offsetToRemove.x, offsetToRemove.y,
                             offsetToRemove.z, (u8)LiquidLevel::Percent0);

  // Update sunlight and block light at position
  removeLight(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);
  checkSunLightAt(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);
  updateSunlight();
  updateBlockLights();
  chunkManager.reloadLightData();

  // Update liquid at position
  checkLiquidPropagation(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);

  playDestroyBlockSound(blockToRemove->getType());

  Chunk* chunkToRebuild = chunkManager.getChunkByBlockOffset(offsetToRemove);
  rebuildChunkNeighbors(chunkToRebuild, const_cast<Vec4*>(&offsetToRemove));

  // Remove up block if it's is vegetation
  const Vec4 upBlockOffset =
      Vec4(offsetToRemove.x, offsetToRemove.y + 1, offsetToRemove.z);

  if (pLevel->BoundCheckMap(upBlockOffset.x, upBlockOffset.y,
                            upBlockOffset.z)) {
    const Blocks upperBlockType = static_cast<Blocks>(pLevel->GetBlockFromMap(
        upBlockOffset.x, upBlockOffset.y, upBlockOffset.z));

    if (blockManager.isVegetation(upperBlockType) ||
        upperBlockType == Blocks::TORCH) {
      Block* _template = blockManager.getBlockTemplateByType(upperBlockType);
      Block* upperBlock = _template->clone();

      u8 visibleFaces =
          VisibleFacesManager::getInstance()->getVisibleFacesByOffset(
              upBlockOffset);

      upperBlock->offset = upBlockOffset;
      upperBlock->index = pLevel->OffsetToIndex(upBlockOffset);
      upperBlock->setVisibleFaces(visibleFaces);
      upperBlock->setVisibleFacesCount(Utils::countSetBits(visibleFaces));
      upperBlock->position = pLevel->offsetToWorldPos(&upperBlock->offset);
      upperBlock->baseColor =
          LightManager::GetLightColorAt(upperBlock->offset, getTargetedFace(),
                                        worldLightModel.sunLightIntensity);

      removeBlock(upperBlock);
      delete upperBlock;
    }
  }
}

bool World::putBlock(const Blocks& blockToPlace, Player* t_player) {
  Vec4 blockOffset = targetBlock->offset;

  // Placing block at invalid position
  if (!pLevel->BoundCheckMap(blockOffset.x, blockOffset.y, blockOffset.z)) {
    return false;
  }

  t_player->playPutBlockAnimation();
  bool blockPlaced = false;

  switch (blockToPlace) {
    case Blocks::TORCH:
      blockPlaced = putTorchBlock();
      break;

    case Blocks::STONE_SLAB:
    case Blocks::BRICKS_SLAB:
    case Blocks::OAK_PLANKS_SLAB:
    case Blocks::SPRUCE_PLANKS_SLAB:
    case Blocks::BIRCH_PLANKS_SLAB:
    case Blocks::ACACIA_PLANKS_SLAB:
    case Blocks::STONE_BRICK_SLAB:
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      blockPlaced = putSlab(blockToPlace, t_player);
      break;

    default:
      blockPlaced = putDefaultBlock(blockToPlace, t_player);
      break;
  }

  if (blockPlaced) playPutBlockSound(blockToPlace);
  return blockPlaced;
}

bool World::putTorchBlock() {
  TargetedFace placementDirection = getTargetedFace();
  Vec4 blockOffset = targetBlock->offset;

  if (placementDirection == TargetedFace::FrontFace) {
    blockOffset.z++;
  } else if (placementDirection == TargetedFace::BackFace) {
    blockOffset.z--;
  } else if (placementDirection == TargetedFace::RightFace) {
    blockOffset.x++;
  } else if (placementDirection == TargetedFace::LeftFace) {
    blockOffset.x--;
  } else if (placementDirection == TargetedFace::TopFace) {
    blockOffset.y++;
  } else if (placementDirection == TargetedFace::BottomFace) {
    blockOffset.y--;
  }

  const bool canReplace =
      pLevel->isPositionEmpty(blockOffset.x, blockOffset.y, blockOffset.z) ||
      pLevel->isGrassAtPosition(blockOffset.x, blockOffset.y, blockOffset.z);

  if (targetBlock->getType() == Blocks::TORCH &&
      placementDirection == TargetedFace::TopFace) {
    return false;
  }

  if (canReplace) {
    // Calc block orientation
    BlockOrientation orientation;

    if (targetBlock->getType() == Blocks::TORCH) {
      orientation = BlockOrientation::Top;
    } else {
      // Torch orientation must be reverse of placement direction
      switch (placementDirection) {
        case TargetedFace::TopFace:
          orientation = BlockOrientation::Top;
          break;
        case TargetedFace::LeftFace:
          orientation = BlockOrientation::East;
          break;
        case TargetedFace::RightFace:
          orientation = BlockOrientation::West;
          break;
        case TargetedFace::FrontFace:
          orientation = BlockOrientation::North;
          break;
        case TargetedFace::BackFace:
          orientation = BlockOrientation::South;
          break;

        case TargetedFace::BottomFace:
        default:
          return false;
      }
    }

    pLevel->SetBlockInMap(blockOffset.x, blockOffset.y, blockOffset.z,
                          static_cast<u8>(Blocks::TORCH));
    pLevel->SetTorchOrientationDataToMap(blockOffset.x, blockOffset.y,
                                         blockOffset.z, orientation);
    checkSunLightAt(blockOffset.x, blockOffset.y, blockOffset.z);

    const auto lightValue = blockManager.getBlockLightValue(Blocks::TORCH);
    addBlockLight(blockOffset.x, blockOffset.y, blockOffset.z, lightValue);

    updateSunlight();
    updateBlockLights();

    // TODO: check if the light is being called multiple times
    // It should be called only once by offset
    chunkManager.reloadLightData();
    updateNeighBorsChunksByAddedBlock(&blockOffset);

    return true;
  }

  return false;
}

/**
 * @param slabToPlace - type of slab to place
 * @param t_player - player reference to player
 */
bool World::putSlab(const Blocks& slabToPlace, Player* t_player) {
  TargetedFace targeted_face = getTargetedFace();
  Vec4 targetPos = ray.at(targetBlock->distance);
  Vec4 newSlabPos = targetBlock->offset;
  SlabOrientation slabOrientation = SlabOrientation::Top;

  // Check if the target block is a slab
  // When the target block IS a slab
  if (blockManager.isSlab(targetBlock->getType())) {
    // Check if the slab is of the same type
    if (targetBlock->getType() == slabToPlace) {
      const auto existing_slab_orientation =
          pLevel->GetSlabOrientationDataFromMap(newSlabPos.x, newSlabPos.y,
                                                newSlabPos.z);
      if ((targeted_face == TargetedFace::TopFace &&
           existing_slab_orientation == SlabOrientation::Bottom) ||
          (targeted_face == TargetedFace::BottomFace &&
           existing_slab_orientation == SlabOrientation::Top)) {
        mergeSlabs(slabToPlace, t_player, newSlabPos);
        return true;
      } else if (targeted_face == TargetedFace::FrontFace ||
                 targeted_face == TargetedFace::BackFace ||
                 targeted_face == TargetedFace::RightFace ||
                 targeted_face == TargetedFace::LeftFace) {
        // newSlabPos.y stays the same
        if (targeted_face == TargetedFace::FrontFace) {
          newSlabPos.z++;
        } else if (targeted_face == TargetedFace::BackFace) {
          newSlabPos.z--;
        } else if (targeted_face == TargetedFace::RightFace) {
          newSlabPos.x++;
        } else if (targeted_face == TargetedFace::LeftFace) {
          newSlabPos.x--;
        }

        const float heightOffset =
            std::fmod(targetPos.y - BLOCK_SIZE, DOUBLE_BLOCK_SIZE);
        slabOrientation = heightOffset > BLOCK_SIZE ? SlabOrientation::Top
                                                    : SlabOrientation::Bottom;

        goto placeSlabOnEmptyOrCombine;  // jump to place slab on empty or
                                         // combine logic
      }
    } else {
      // If the slab is of a different type, place the new slab on top of or
      // below the existing slab based on the targeted face

      if (targeted_face == TargetedFace::TopFace) {
        newSlabPos.y++;
        slabOrientation = SlabOrientation::Bottom;
      } else if (targeted_face == TargetedFace::BottomFace) {
        newSlabPos.y--;
        slabOrientation = SlabOrientation::Top;
      } else if (targeted_face == TargetedFace::FrontFace) {
        newSlabPos.z++;
      } else if (targeted_face == TargetedFace::BackFace) {
        newSlabPos.z--;
      } else if (targeted_face == TargetedFace::RightFace) {
        newSlabPos.x++;
      } else if (targeted_face == TargetedFace::LeftFace) {
        newSlabPos.x--;
      }

      const float heightOffset =
          std::fmod(targetPos.y - BLOCK_SIZE, DOUBLE_BLOCK_SIZE);
      slabOrientation = heightOffset > BLOCK_SIZE ? SlabOrientation::Top
                                                  : SlabOrientation::Bottom;
      goto placeSlabOnEmptyOrCombine;  // jump to place slab on empty or
                                       // combine logic
    }

    return false;
  }

  // When the target block IS NOT a slab
  // Calculate slab orientation and block offset based on targeted face

  if (targeted_face == TargetedFace::TopFace) {
    newSlabPos.y++;
    slabOrientation = SlabOrientation::Bottom;
  } else if (targeted_face == TargetedFace::BottomFace) {
    newSlabPos.y--;
    slabOrientation = SlabOrientation::Top;
  } else if (targeted_face == TargetedFace::FrontFace ||
             targeted_face == TargetedFace::BackFace ||
             targeted_face == TargetedFace::RightFace ||
             targeted_face == TargetedFace::LeftFace) {
    // newSlabPos.y stays the same
    if (targeted_face == TargetedFace::FrontFace) {
      newSlabPos.z++;
    } else if (targeted_face == TargetedFace::BackFace) {
      newSlabPos.z--;
    } else if (targeted_face == TargetedFace::RightFace) {
      newSlabPos.x++;
    } else if (targeted_face == TargetedFace::LeftFace) {
      newSlabPos.x--;
    }

    const float heightOffset =
        std::fmod(targetPos.y - BLOCK_SIZE, DOUBLE_BLOCK_SIZE);
    slabOrientation = heightOffset > BLOCK_SIZE ? SlabOrientation::Top
                                                : SlabOrientation::Bottom;
  }

// When placing a slab on an empty block or combining
// Label for jumping to this point
placeSlabOnEmptyOrCombine:

  // Prevent to put a block at the player position;
  Vec4 newBlockPos = pLevel->offsetToWorldPos(newSlabPos);

  M4x4 tempModel = M4x4();
  tempModel.identity();
  tempModel.scaleX(BLOCK_SIZE);
  tempModel.scaleZ(BLOCK_SIZE);
  tempModel.scaleY(HALF_BLOCK_SIZE);
  tempModel.translate(newBlockPos);

  BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(&newSlabPos);
  BBox tempBBox = rawBBox->getTransformed(tempModel);
  BBox newBlockBBox = BBox(tempBBox.vertices, tempBBox.getVertexCount());
  BBox playerBBox = t_player->getHitBox();

  // Return if collides to player
  if (Utils::AABBCollides(&newBlockBBox, &playerBBox)) {
    return false;
  }

  if (pLevel->isReplaceableBySolidBlock(newSlabPos.x, newSlabPos.y,
                                        newSlabPos.z)) {
    pLevel->SetSlabOrientationDataToMap(newSlabPos.x, newSlabPos.y,
                                        newSlabPos.z, slabOrientation);
    placeBlockAt(slabToPlace, newSlabPos);
    return true;
  } else {
    // The space is not empty, check if the block is already a slab of the
    // same type
    const Blocks existing_block = static_cast<Blocks>(
        pLevel->GetBlockFromMap(newSlabPos.x, newSlabPos.y, newSlabPos.z));
    if (existing_block == slabToPlace) {
      const auto existing_slab_orientation =
          pLevel->GetSlabOrientationDataFromMap(newSlabPos.x, newSlabPos.y,
                                                newSlabPos.z);

      // if existing_block is a SLAB and existing_block.type is the same as
      // slab_type:
      //    If you try to place a bottom slab on an existing bottom slab, it
      //    should become a full block This is a special case for double slabs
      if ((slabOrientation == SlabOrientation::Top &&
           existing_slab_orientation == SlabOrientation::Bottom)) {
        mergeSlabs(slabToPlace, t_player, newSlabPos);
        return true;
      } else if ((slabOrientation == SlabOrientation::Bottom &&
                  existing_slab_orientation == SlabOrientation::Top)) {
        mergeSlabs(slabToPlace, t_player, newSlabPos);
        return true;
      }
      return false;  // Cannot place slab
    }
  }

  return false;  // Cannot place slab
}

void World::mergeSlabs(const Blocks& slabToPlace, Player* t_player,
                       const Vec4& offsetToMerge) {
  Blocks newBlock;

  switch (slabToPlace) {
    case Blocks::STONE_SLAB:
      newBlock = Blocks::STONE_BLOCK;
      break;
    case Blocks::BRICKS_SLAB:
      newBlock = Blocks::BRICKS_BLOCK;
      break;
    case Blocks::OAK_PLANKS_SLAB:
      newBlock = Blocks::OAK_PLANKS_BLOCK;
      break;
    case Blocks::SPRUCE_PLANKS_SLAB:
      newBlock = Blocks::SPRUCE_PLANKS_BLOCK;
      break;
    case Blocks::BIRCH_PLANKS_SLAB:
      newBlock = Blocks::BIRCH_PLANKS_BLOCK;
      break;
    case Blocks::ACACIA_PLANKS_SLAB:
      newBlock = Blocks::ACACIA_PLANKS_BLOCK;
      break;
    case Blocks::STONE_BRICK_SLAB:
      newBlock = Blocks::STONE_BRICK_BLOCK;
      break;
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
      newBlock = Blocks::CRACKED_STONE_BRICKS_BLOCK;
      break;
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      newBlock = Blocks::MOSSY_STONE_BRICKS_BLOCK;
      break;

    default:
      newBlock = Blocks::AIR_BLOCK;
      TYRA_ERROR("Not valid double slab!");
      break;
  }

  placeBlockAt(newBlock, offsetToMerge);

  // Force to rebuild the target block because the merged slab has the same
  // index
  buildTargetBlockDrawData();
}

bool World::putDefaultBlock(const Blocks blockToPlace, Player* t_player) {
  TargetedFace placementDirection = getTargetedFace();
  Vec4 blockOffset = targetBlock->offset;

  if (placementDirection == TargetedFace::FrontFace) {
    blockOffset.z++;
  } else if (placementDirection == TargetedFace::BackFace) {
    blockOffset.z--;
  } else if (placementDirection == TargetedFace::RightFace) {
    blockOffset.x++;
  } else if (placementDirection == TargetedFace::LeftFace) {
    blockOffset.x--;
  } else if (placementDirection == TargetedFace::TopFace) {
    blockOffset.y++;
  } else if (placementDirection == TargetedFace::BottomFace) {
    blockOffset.y--;
  }

  Vec4 newBlockPos = blockOffset * DOUBLE_BLOCK_SIZE;

  // Prevent to put a block at the player position;
  M4x4 tempModel = M4x4();
  tempModel.identity();
  tempModel.scale(BLOCK_SIZE);
  tempModel.translate(newBlockPos);

  BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(&blockOffset);
  BBox tempBBox = rawBBox->getTransformed(tempModel);
  BBox finalBBox = BBox(tempBBox.vertices, tempBBox.getVertexCount());

  Vec4 newBlockPosMin;
  Vec4 newBlockPosMax;
  finalBBox.getMinMax(&newBlockPosMin, &newBlockPosMax);

  Vec4 minPlayerCorner;
  Vec4 maxPlayerCorner;
  t_player->getHitBox().getMinMax(&minPlayerCorner, &maxPlayerCorner);

  // Will Collide to player?
  if (newBlockPosMax.x > minPlayerCorner.x &&
      newBlockPosMin.x < maxPlayerCorner.x &&
      newBlockPosMax.z > minPlayerCorner.z &&
      newBlockPosMin.z < maxPlayerCorner.z &&
      newBlockPosMax.y > minPlayerCorner.y &&
      newBlockPosMin.y < maxPlayerCorner.y) {
    return false;  // Return on collision
  }

  const u8 canReplace = pLevel->isReplaceableBySolidBlock(
      blockOffset.x, blockOffset.y, blockOffset.z);

  if (canReplace) {
    placeBlockAt(blockToPlace, blockOffset);
    return true;
  }

  return false;
}

void World::stopBreakTargetBlock() {
  _isBreakingBlock = false;
  breaking_time_pessed = 0;
  if (targetBlock) {
    targetBlock->damage = 0;
    buildTargetBlockDrawData();
  }
}

void World::breakTargetBlock(const float& deltaTime) {
  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;
    const auto breakingTime = blockManager.getBlockBreakingTime(targetBlock);
    if (breaking_time_pessed >= breakingTime) {
      // Remove block;
      removeBlock(targetBlock);
      delete targetBlock;
      targetBlock = nullptr;

      // Target block has changed, reseting the pressed time;
      breaking_time_pessed = 0;
    } else {
      // Update damage overlay
      targetBlock->damage = breaking_time_pessed / breakingTime * 100;

      if (lastTimeCreatedParticle > 0.2) {
        particlesManager.createBlockParticleBatch(targetBlock, 4);
        lastTimeCreatedParticle = 0;
      } else {
        lastTimeCreatedParticle += deltaTime;
      }

      if (lastTimePlayedBreakingSfx > 0.3F) {
        playBreakingBlockSound(targetBlock->getType());
        lastTimePlayedBreakingSfx = 0;
      } else {
        lastTimePlayedBreakingSfx += deltaTime;
      }
    }

    return;
  }

  breaking_time_pessed = 0;
  _isBreakingBlock = true;
}

void World::breakTargetBlockInCreativeMode(const float& deltaTime) {
  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;
    const auto breakingTime = BREAKING_TIME_IN_CREATIVE_MODE;
    if (breaking_time_pessed >= breakingTime) {
      // Remove block;
      removeBlock(targetBlock);
      delete targetBlock;
      targetBlock = nullptr;

      // Target block has changed, reseting the pressed time;
      breaking_time_pessed = 0;
    } else {
      // Update damage overlay
      targetBlock->damage = breaking_time_pessed / breakingTime * 100;

      if (lastTimeCreatedParticle > 0.2) {
        particlesManager.createBlockParticleBatch(targetBlock, 3);
        lastTimeCreatedParticle = 0;
      } else {
        lastTimeCreatedParticle += deltaTime;
      }

      if (lastTimePlayedBreakingSfx > 0.3F) {
        playBreakingBlockSound(targetBlock->getType());
        lastTimePlayedBreakingSfx = 0;
      } else {
        lastTimePlayedBreakingSfx += deltaTime;
      }
    }

    return;
  }

  breaking_time_pessed = 0;
  _isBreakingBlock = true;
}

void World::playPutBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getDigSoundByBlockType(blockType);
    if (blockSfxModel) {
      SoundManager* pSoundManager = SoundManager::getInstance();

      const int ch = pSoundManager->getAvailableChannel();
      SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getPlaceSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      pSoundManager->setSfxVolume(config->_volume, ch);
      pSoundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void World::playDestroyBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getBrokenSoundByBlockType(blockType);

    if (blockSfxModel) {
      SoundManager* pSoundManager = SoundManager::getInstance();
      const int ch = pSoundManager->getAvailableChannel();
      SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getBrokenSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      pSoundManager->setSfxVolume(config->_volume, ch);
      pSoundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void World::playBreakingBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getDigSoundByBlockType(blockType);

    if (blockSfxModel) {
      SoundManager* pSoundManager = SoundManager::getInstance();
      const int ch = pSoundManager->getAvailableChannel();
      SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getBreakingSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      pSoundManager->setSfxVolume(config->_volume, ch);
      pSoundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void World::playFlowingWaterSound() {
  SfxBlockModel waterSfxModel = SfxBlockModel(
      Blocks::WATER_BLOCK, SoundFxCategory::Liquid, SoundFX::Water);

  SoundManager* pSoundManager = SoundManager::getInstance();
  const int ch = pSoundManager->getAvailableChannel();
  SfxLibrarySound* sound = pSoundManager->getSound(&waterSfxModel);

  const u8 pitch = Tyra::Math::randomi(50, 150);
  const u8 volume = Tyra::Math::randomi(75, 100);

  sound->_sound->pitch = pitch;
  pSoundManager->setSfxVolume(volume, ch);
  pSoundManager->playSfx(sound, ch);
}

u8 World::isCrossedBlock(Blocks block_type) {
  return block_type == Blocks::POPPY_FLOWER ||
         block_type == Blocks::DANDELION_FLOWER || block_type == Blocks::GRASS;
}

void World::rebuildChunkNeighbors(Chunk* t_chunk, Vec4* moddedOffset) {
  if (g_debug_mode) t_chunk->buildingTimeStart = clock();

  Vec4 bottom = *moddedOffset + DOWN_VEC;
  if (!t_chunk->containsBlock(&bottom)) {
    Chunk* bottomChunk = chunkManager.getChunkByBlockOffset(bottom);
    if (bottomChunk && bottomChunk->isLoaded()) bottomChunk->rebuild();
  }

  Vec4 top = *moddedOffset + UP_VEC;
  if (!t_chunk->containsBlock(&top)) {
    Chunk* topChunk = chunkManager.getChunkByBlockOffset(top);
    if (topChunk && topChunk->isLoaded()) topChunk->rebuild();
  }

  Vec4 right = *moddedOffset + RIGHT_VEC;
  if (t_chunk->containsBlock(&right)) {
    Chunk* rightChunk = chunkManager.getChunkByBlockOffset(right);
    if (rightChunk && rightChunk->isLoaded()) rightChunk->rebuild();
  }

  Vec4 left = *moddedOffset + LEFT_VEC;
  if (t_chunk->containsBlock(&left)) {
    Chunk* leftChunk = chunkManager.getChunkByBlockOffset(left);
    if (leftChunk && leftChunk->isLoaded()) leftChunk->rebuild();
  }

  Vec4 front = *moddedOffset + FRONT_VEC;
  if (t_chunk->containsBlock(&front)) {
    Chunk* frontChunk = chunkManager.getChunkByBlockOffset(front);
    if (frontChunk && frontChunk->isLoaded()) frontChunk->rebuild();
  }

  Vec4 back = *moddedOffset + BACK_VEC;
  if (t_chunk->containsBlock(&back)) {
    Chunk* backChunk = chunkManager.getChunkByBlockOffset(back);
    if (backChunk && backChunk->isLoaded()) backChunk->rebuild();
  }

  t_chunk->rebuild();

  if (g_debug_mode) {
    t_chunk->timeToBuild =
        ((float)(clock() - t_chunk->buildingTimeStart)) / CLOCKS_PER_SEC;
    printf("Time to sync build chunk fragment %i: %f\n", t_chunk->id,
           t_chunk->timeToBuild);
  }
}

void World::updateTargetBlock(Camera* t_camera, Player* t_player) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderBlockDamage == false) return;
#endif  // end if DEBUG_MODE

  u32 _lastTargetBlockId = 999999;

  if (targetBlock) {
    _lastTargetBlockId = targetBlock->index;
    delete targetBlock;
    targetBlock = nullptr;
  };

  const Vec4 origin =
      *t_player->getPosition() + Vec4(0.0f, t_camera->getCamY(), 0.0f);

  ray.origin = origin;
  ray.direction.set(t_camera->unitCirclePosition.getNormalized());

  // Broad phase raycast
  std::vector<LevelIntersectQueryResult> tempResult = {};
  pLevel->getIntersectedBlocks(ray.origin, ray.at(MAX_RANGE_PICKER),
                               &tempResult);

  // Narrow phase raycast
  for (const auto& result : tempResult) {
    Block* targetTemplate = blockManager.getBlockTemplateByType(
        static_cast<Blocks>(result.blockType));
    if (targetTemplate->isBreakable()) {
      BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(
          const_cast<Vec4*>(&result.offset));
      M4x4 model = ModelBuilder_BuildModel(const_cast<Vec4*>(&result.offset));
      BBox blockBBox = rawBBox->getTransformed(model);

      Vec4 min, max;
      blockBBox.getMinMax(&min, &max);

      u8 visibleFaces =
          VisibleFacesManager::getInstance()->getVisibleFacesByOffset(
              result.offset);

      // Check if the ray intersects the bounding box
      float distance = 0.0f;
      if (ray.intersectBox(min, max, &distance)) {
        // Create the targetBlock
        targetBlock = targetTemplate->clone();
        targetBlock->index = pLevel->OffsetToIndex(result.offset);
        targetBlock->distance = distance;
        targetBlock->setHitPosition(ray.at(distance));
        targetBlock->setIsTarget(true);
        targetBlock->setVisibleFaces(visibleFaces);
        targetBlock->setVisibleFacesCount(Utils::countSetBits(visibleFaces));
        targetBlock->position = pLevel->offsetToWorldPos(&result.offset);
        targetBlock->bbox = new BBox(blockBBox);
        targetBlock->minCorner.set(min);
        targetBlock->maxCorner.set(max);
        targetBlock->offset = result.offset;
        targetBlock->baseColor =
            LightManager::GetLightColorAt(result.offset, getTargetedFace(),
                                          worldLightModel.sunLightIntensity);
        M4x4::copy(&targetBlock->model, model);

        if (targetBlock->index != _lastTargetBlockId) {
          breaking_time_pessed = 0;
          buildTargetBlockDrawData();
        }

        break;
      }
    }
  }
}

void World::buildTargetBlockDrawData() {
  TYRA_ASSERT(targetBlock != nullptr, "No target block to build draw data!");

  clearTargetBlockDrawData();

  const u8 size = Utils::countSetBits(targetBlock->getVisibleFaces()) *
                  VertexBlockData::FACES_COUNT;
  if (_targetBlockVertices.capacity() < size) {
    _targetBlockVertices.reserve(size);
    _targetBlockColors.reserve(size);
    _targetBlockUVMap.reserve(size);
  }

  // Inflate the target block a bit to avoid z-fighting
  CustomMeshOptions options = {.scale = 1.01f};
  MeshBuilder_BuildMesh(&targetBlock->offset, targetBlock->getVisibleFaces(), 0,
                        &_targetBlockVertices, &_targetBlockColors,
                        &_targetBlockUVMap, &worldLightModel, pLevel, &options);

  for (size_t i = 0; i < size; i++) {
    LightManager::IntensifyColor(&_targetBlockColors[i], 1.35f);
  }
}

void World::updateBlockDamage() {
  _targetBlockUVMap.clear();

  const u8 V = 15;
  const u8 U = floor(targetBlock->damage / 10);
  const float _scale = 1.0F / 16.0F;
  const Vec4 UVScale = Vec4(_scale, _scale, 1.0F, 0.0F);
  const u8 size =
      targetBlock->packed.visibleFacesCount * VertexBlockData::FACES_COUNT;

  u8 idx = 0;
  const u8 faces = size / 6;

  for (size_t i = 0; i < faces; i++) {
    _targetBlockUVMap[idx++] = (Vec4(U, (V + 1.0F), 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] = (Vec4((U + 1.0F), V, 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] =
        (Vec4((U + 1.0F), (V + 1.0F), 1.0F, 0.0F) * UVScale);

    _targetBlockUVMap[idx++] = (Vec4(U, (V + 1.0F), 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] = (Vec4(U, V, 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] = (Vec4((U + 1.0F), V, 1.0F, 0.0F) * UVScale);
  }

  for (size_t i = 0; i < size; i++) {
    _targetBlockColors[i].a = 70.0f;
  };
}

void World::clearTargetBlockDrawData() {
  _targetBlockVertices.clear();
  _targetBlockColors.clear();
  _targetBlockUVMap.clear();
}

void World::setDrawDistance(const u8& drawDistanceInChunks) {
  TYRA_LOG("Setting draw distance to ", static_cast<int>(drawDistanceInChunks),
           " chunks");
  if (drawDistanceInChunks >= MIN_DRAW_DISTANCE &&
      drawDistanceInChunks <= MAX_DRAW_DISTANCE) {
    worldOptions.drawDistance = drawDistanceInChunks;
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

void World::checkLiquidPropagation(uint16_t x, uint16_t y, uint16_t z) {
  if (pLevel->BoundCheckMap(x - 1, y, z)) {
    Blocks nl = static_cast<Blocks>(pLevel->GetBlockFromMap(x - 1, y, z));
    u8 level = pLevel->GetLiquidDataFromMap(x - 1, y, z);

    if (nl == Blocks::WATER_BLOCK || nl == Blocks::LAVA_BLOCK) {
      addLiquid(x - 1, y, z, (u8)nl, level);
    }
  }

  if (pLevel->BoundCheckMap(x + 1, y, z)) {
    Blocks nr = static_cast<Blocks>(pLevel->GetBlockFromMap(x + 1, y, z));
    u8 level = pLevel->GetLiquidDataFromMap(x + 1, y, z);

    if (nr == Blocks::WATER_BLOCK || nr == Blocks::LAVA_BLOCK) {
      addLiquid(x + 1, y, z, (u8)nr, level);
    }
  }

  if (pLevel->BoundCheckMap(x, y - 1, z)) {
    Blocks nd = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y - 1, z));
    u8 level = pLevel->GetLiquidDataFromMap(x, y - 1, z);

    if (nd == Blocks::WATER_BLOCK || nd == Blocks::LAVA_BLOCK) {
      addLiquid(x, y - 1, z, (u8)nd, level);
    }
  }

  if (pLevel->BoundCheckMap(x, y, z + 1)) {
    Blocks nf = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z + 1));
    u8 level = pLevel->GetLiquidDataFromMap(x, y, z + 1);

    if (nf == Blocks::WATER_BLOCK || nf == Blocks::LAVA_BLOCK) {
      addLiquid(x, y, z + 1, (u8)nf, level);
    }
  }

  if (pLevel->BoundCheckMap(x, y, z - 1)) {
    Blocks nb = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z - 1));
    u8 level = pLevel->GetLiquidDataFromMap(x, y, z - 1);

    if (nb == Blocks::WATER_BLOCK || nb == Blocks::LAVA_BLOCK) {
      addLiquid(x, y, z - 1, (u8)nb, level);
    }
  }
}

void World::addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level,
                      u8 orientation) {
  if (level > (u8)LiquidLevel::Percent0) {
    if (type == (u8)Blocks::WATER_BLOCK) {
      waterBfsQueue.emplace(x, y, z, level);
    } else if (type == (u8)Blocks::LAVA_BLOCK) {
      lavaBfsQueue.emplace(x, y, z, level);
      addBlockLight(x, y, z, 15);
      updateBlockLights();
    }

    pLevel->SetBlockInMap(x, y, z, type);
    pLevel->SetLiquidDataToMap(x, y, z, level);

    const auto prevDir = pLevel->GetLiquidOrientationDataFromMap(x, y, z);

    // Fix traversal orientation to linear
    // It two traversal flux encounter each other, it will become a linear
    // flux
    if (((u8)LiquidOrientation::NorthEast == orientation &&
         LiquidOrientation::NorthWest == prevDir) ||
        ((u8)LiquidOrientation::NorthWest == orientation &&
         LiquidOrientation::NorthEast == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::North);
    } else if (((u8)LiquidOrientation::NorthEast == orientation &&
                LiquidOrientation::SouthEast == prevDir) ||
               ((u8)LiquidOrientation::SouthEast == orientation &&
                LiquidOrientation::NorthEast == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::East);
    } else if (((u8)LiquidOrientation::SouthEast == orientation &&
                LiquidOrientation::SouthWest == prevDir) ||
               ((u8)LiquidOrientation::SouthWest == orientation &&
                LiquidOrientation::SouthEast == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::South);
    } else if (((u8)LiquidOrientation::NorthWest == orientation &&
                LiquidOrientation::SouthWest == prevDir) ||
               ((u8)LiquidOrientation::SouthWest == orientation &&
                LiquidOrientation::NorthWest == prevDir)) {
      pLevel->SetLiquidOrientationDataToMap(x, y, z, LiquidOrientation::West);
    } else {
      pLevel->SetLiquidOrientationDataToMap(
          x, y, z, static_cast<LiquidOrientation>(orientation));
    }

    Chunk* moddedChunk = chunkManager.getChunkByBlockOffset(Vec4(x, y, z));
    if (moddedChunk && moddedChunk->isLoaded()) {
      affectedChunksIdByLiquidPropagation.insert(moddedChunk);
    }
  }
}

void World::addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level) {
  addLiquid(x, y, z, type, level, (u8)LiquidOrientation::East);
}

void World::removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type) {
  u8 liquidLevel = pLevel->GetLiquidDataFromMap(x, y, z);
  removeLiquid(x, y, z, type, liquidLevel);
}

void World::removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type,
                         u8 level) {
  if (level > (u8)LiquidLevel::Percent0) {
    if (type == (u8)Blocks::WATER_BLOCK) {
      waterRemovalBfsQueue.emplace(x, y, z, level);
    } else if (type == (u8)Blocks::LAVA_BLOCK) {
      lavaRemovalBfsQueue.emplace(x, y, z, level);
      removeLight(x, y, z);
      updateBlockLights();
    }

    pLevel->SetLiquidDataToMap(x, y, z, level);
  } else {
    pLevel->SetBlockInMap(x, y, z, (u8)Blocks::AIR_BLOCK);
    pLevel->SetLiquidDataToMap(x, y, z, (u8)LiquidLevel::Percent0);
  }

  Chunk* moddedChunk = chunkManager.getChunkByBlockOffset(Vec4(x, y, z));
  if (moddedChunk) {
    affectedChunksIdByLiquidPropagation.insert(moddedChunk);
  }
}

void World::initLiquidExpansion() {
  TYRA_LOG("Initiating water propagation...");

  for (int x = 0; x < pLevel->map.length; x++) {
    for (int z = 0; z < pLevel->map.width; z++) {
      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));

        if (b == Blocks::AIR_BLOCK) {
          auto liquidValue = LiquidLevel::Percent100;

          if (pLevel->BoundCheckMap(x - 1, y, z)) {
            auto type = pLevel->GetBlockFromMap(x - 1, y, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x - 1, y, z, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x + 1, y, z)) {
            auto type = pLevel->GetBlockFromMap(x + 1, y, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x + 1, y, z, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x, y - 1, z)) {
            auto type = pLevel->GetBlockFromMap(x, y - 1, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y - 1, z, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x, y, z - 1)) {
            auto type = pLevel->GetBlockFromMap(x, y, z - 1);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y, z - 1, type, liquidValue);
          } else if (pLevel->BoundCheckMap(x, y, z + 1)) {
            auto type = pLevel->GetBlockFromMap(x, y, z + 1);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y, z + 1, type, liquidValue);
          }
        }
      }
    }
  }
}

void World::updateLiquidWater() {
  propagateWaterRemovalQueue();
  propagateWaterAddQueue();

  // TODO: check if is near flowing water
  // if (lastTimePlayedWaterSound > waterSoundTimeCounter) {
  //   playFlowingWaterSound();
  //   const uint16_t delayToPlayNextTime = Tyra::Math::randomi(1, 15) * 1000;
  //   waterSoundTimeCounter = waterSoundDuration + delayToPlayNextTime;
  //   lastTimePlayedWaterSound = 0;
  // } else {
  //   lastTimePlayedWaterSound += deltaTime;
  // }
}

// FIX: lava spread only 3 blocks in overworld
void World::updateLiquidLava() {
  propagateLavaRemovalQueue();
  propagateLavaAddQueue();
}

void World::propagateWaterRemovalQueue() {
  if (waterRemovalBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = waterRemovalBfsQueue.size();
    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      Node liquidNode = waterRemovalBfsQueue.front();

      // get the index
      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;
      uint8_t liquidValue = liquidNode.val - 1;

      waterRemovalBfsQueue.pop();

      if (pLevel->BoundCheckMap(nx + 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx + 1, ny, nz) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx + 1, ny, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (pLevel->BoundCheckMap(nx, ny, nz + 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz + 1) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx, ny, nz + 1, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (pLevel->BoundCheckMap(nx - 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx - 1, ny, nz) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx - 1, ny, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (pLevel->BoundCheckMap(nx, ny - 1, nz) &&
          pLevel->GetBlockFromMap(nx, ny - 1, nz) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx, ny - 1, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (pLevel->BoundCheckMap(nx, ny, nz - 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz - 1) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx, ny, nz - 1, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (liquidValue > (u8)LiquidLevel::Percent0) {
        waterRemovalBfsQueue.emplace(nx, ny, nz, liquidValue);
      }
    }
  }
}

void World::propagateLavaRemovalQueue() {
  if (lavaRemovalBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = lavaRemovalBfsQueue.size();
    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      Node liquidNode = lavaRemovalBfsQueue.front();

      // get the index
      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;

      s8 nextLevel = (u8)LiquidLevel::Percent0;
      if (liquidNode.val == (u8)LiquidLevel::Percent100) {
        nextLevel = (u8)LiquidLevel::Percent75;
      } else if (liquidNode.val == (u8)LiquidLevel::Percent75) {
        nextLevel = (u8)LiquidLevel::Percent50;
      } else if (liquidNode.val == (u8)LiquidLevel::Percent50) {
        nextLevel = (u8)LiquidLevel::Percent25;
      } else {
        return;
      }

      lavaRemovalBfsQueue.pop();

      if (pLevel->BoundCheckMap(nx + 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx + 1, ny, nz) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx + 1, ny, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (pLevel->BoundCheckMap(nx, ny, nz + 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz + 1) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx, ny, nz + 1, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (pLevel->BoundCheckMap(nx - 1, ny, nz) &&
          pLevel->GetBlockFromMap(nx - 1, ny, nz) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx - 1, ny, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (pLevel->BoundCheckMap(nx, ny - 1, nz) &&
          pLevel->GetBlockFromMap(nx, ny - 1, nz) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx, ny - 1, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (pLevel->BoundCheckMap(nx, ny, nz - 1) &&
          pLevel->GetBlockFromMap(nx, ny, nz - 1) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx, ny, nz - 1, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (nextLevel > (u8)LiquidLevel::Percent0) {
        lavaRemovalBfsQueue.emplace(nx, ny, nz, nextLevel);
      }
    }
  }
}

void World::floodFillLiquidRemove(uint16_t x, uint16_t y, uint16_t z, u8 type,
                                  u8 level) {
  u8 neighborLevel = pLevel->GetLiquidDataFromMap(x, y, z);

  if (neighborLevel <= level + 1) {
    removeLiquid(x, y, z, type, level);
  } else if (neighborLevel > level) {
    addLiquid(x, y, z, type, neighborLevel);
  }
}

void World::floodFillLiquidAdd(uint16_t x, uint16_t y, uint16_t z, u8 type,
                               u8 nextLevel, u8 orientation) {
  if (pLevel->GetLiquidDataFromMap(x, y, z) + 1 < nextLevel) {
    addLiquid(x, y, z, type, nextLevel, orientation);
  }
}

void World::propagateWaterAddQueue() {
  if (waterBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = waterBfsQueue.size();

    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      auto liquidNode = waterBfsQueue.front();

      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;

      waterBfsQueue.pop();

      s16 nextLevel = liquidNode.val - 1;
      u8 type = static_cast<u8>(Blocks::WATER_BLOCK);

      if (canPropagateLiquid(nx, ny - 1, nz)) {
        // If down block is air, keep propagating until hit a surface;
        floodFillLiquidAdd(nx, ny - 1, nz, type, LiquidLevel::Percent100,
                           (u8)LiquidOrientation::East);
        return;
      }

      if (nextLevel <= (u8)LiquidLevel::Percent0) {
        return;
      }

      if (canPropagateLiquid(nx + 1, ny, nz)) {
        floodFillLiquidAdd(nx + 1, ny, nz, type, nextLevel,
                           (u8)LiquidOrientation::North);
      }

      if (canPropagateLiquid(nx, ny, nz + 1)) {
        floodFillLiquidAdd(nx, ny, nz + 1, type, nextLevel,
                           (u8)LiquidOrientation::East);
      }

      if (canPropagateLiquid(nx - 1, ny, nz)) {
        floodFillLiquidAdd(nx - 1, ny, nz, type, nextLevel,
                           (u8)LiquidOrientation::South);
      }

      if (canPropagateLiquid(nx, ny, nz - 1)) {
        floodFillLiquidAdd(nx, ny, nz - 1, type, nextLevel,
                           (u8)LiquidOrientation::West);
      }

      if (canPropagateLiquid(nx + 1, ny, nz + 1)) {
        floodFillLiquidAdd(nx + 1, ny, nz + 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::NorthEast);
      }

      if (canPropagateLiquid(nx + 1, ny, nz - 1)) {
        floodFillLiquidAdd(nx + 1, ny, nz - 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::NorthWest);
      }

      if (canPropagateLiquid(nx - 1, ny, nz + 1)) {
        floodFillLiquidAdd(nx - 1, ny, nz + 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::SouthEast);
      }

      if (canPropagateLiquid(nx - 1, ny, nz - 1)) {
        floodFillLiquidAdd(nx - 1, ny, nz - 1, type, nextLevel - 1,
                           (u8)LiquidOrientation::SouthWest);
      }
    }
  }
}

void World::propagateLavaAddQueue() {
  if (lavaBfsQueue.empty() == false) {
    const auto countToUpdateThisTick = lavaBfsQueue.size();
    for (size_t i = 0; i < countToUpdateThisTick; i++) {
      auto liquidNode = lavaBfsQueue.front();

      uint16_t nx = liquidNode.x;
      uint16_t ny = liquidNode.y;
      uint16_t nz = liquidNode.z;

      lavaBfsQueue.pop();

      s8 nextLevel = getNextLavaLevel(liquidNode.val);
      u8 type = (u8)Blocks::LAVA_BLOCK;

      if (canPropagateLiquid(nx, ny - 1, nz)) {
        // If down block is air, keep propagating until hit a surface;
        floodFillLiquidAdd(nx, ny - 1, nz, type, LiquidLevel::Percent100,
                           (u8)BlockOrientation::East);
        return;
      }

      if (nextLevel <= (u8)LiquidLevel::Percent0) return;

      if (canPropagateLiquid(nx + 1, ny, nz)) {
        floodFillLiquidAdd(nx + 1, ny, nz, type, nextLevel,
                           (u8)BlockOrientation::North);
      }

      if (canPropagateLiquid(nx, ny, nz + 1)) {
        floodFillLiquidAdd(nx, ny, nz + 1, type, nextLevel,
                           (u8)BlockOrientation::East);
      }

      if (canPropagateLiquid(nx - 1, ny, nz)) {
        floodFillLiquidAdd(nx - 1, ny, nz, type, nextLevel,
                           (u8)BlockOrientation::South);
      }

      if (canPropagateLiquid(nx, ny, nz - 1)) {
        floodFillLiquidAdd(nx, ny, nz - 1, type, nextLevel,
                           (u8)BlockOrientation::West);
      }

      if (canPropagateLiquid(nx + 1, ny, nz + 1)) {
        floodFillLiquidAdd(nx + 1, ny, nz + 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::NorthEast);
      }

      if (canPropagateLiquid(nx + 1, ny, nz - 1)) {
        floodFillLiquidAdd(nx + 1, ny, nz - 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::NorthWest);
      }

      if (canPropagateLiquid(nx - 1, ny, nz + 1)) {
        floodFillLiquidAdd(nx - 1, ny, nz + 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::SouthEast);
      }

      if (canPropagateLiquid(nx - 1, ny, nz - 1)) {
        floodFillLiquidAdd(nx - 1, ny, nz - 1, type,
                           getNextLavaLevel(nextLevel),
                           (u8)LiquidOrientation::SouthWest);
      }
    }
  }
}

const s8 World::getNextLavaLevel(const s8 currentLevel) {
  s8 nextLevel = (u8)LiquidLevel::Percent0;
  if (currentLevel == (u8)LiquidLevel::Percent100) {
    nextLevel = (u8)LiquidLevel::Percent75;
  } else if (currentLevel == (u8)LiquidLevel::Percent75) {
    nextLevel = (u8)LiquidLevel::Percent50;
  } else if (currentLevel == (u8)LiquidLevel::Percent50) {
    nextLevel = (u8)LiquidLevel::Percent25;
  } else if (currentLevel == (u8)LiquidLevel::Percent25) {
    nextLevel = (u8)LiquidLevel::Percent0;
  }

  return nextLevel;
}

void World::updateChunksAffectedByLiquidPropagation() {
  for (auto chunkPtr : affectedChunksIdByLiquidPropagation) {
    Chunk* moddedChunk = chunkPtr;
    if (moddedChunk) {
      if (moddedChunk->isLoaded()) {
        moddedChunk->rebuild();
      } else {
        moddedChunk->build();
      }

      // moddedChunk->clear();
      // buildChunk(moddedChunk);
    }
  }

  affectedChunksIdByLiquidPropagation.clear();
}

u8 World::canPropagateLiquid(uint16_t x, uint16_t y, uint16_t z) {
  if (!pLevel->BoundCheckMap(x, y, z)) return false;
  const u8 type = pLevel->GetBlockFromMap(x, y, z);
  return type == (u8)Blocks::AIR_BLOCK || type == (u8)Blocks::GRASS ||
         type == (u8)Blocks::POPPY_FLOWER || type == (u8)Blocks::TORCH ||
         type == (u8)Blocks::DANDELION_FLOWER;
}

void World::updateSunlight() {
  if (sunlightRemovalBfsQueue.empty() == false) {
    propagateSunlightRemovalQueue();
  }

  if (sunlightBfsQueue.empty() == false) {
    propagateSunLightAddBFSQueue();
  }
}

void World::propagateSunLightAddBFSQueue() {
  while (!sunlightBfsQueue.empty()) {
    auto lightNode = sunlightBfsQueue.front();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    sunlightBfsQueue.pop();

    int nextLightValue = lightValue - 1;

    if (nextLightValue < 0) {
      continue;
    }

    if (pLevel->BoundCheckMap(nx + 1, ny, nz)) {
      floodFillSunlightAdd(nx + 1, ny, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny + 1, nz)) {
      floodFillSunlightAdd(nx, ny + 1, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz + 1)) {
      floodFillSunlightAdd(nx, ny, nz + 1, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx - 1, ny, nz)) {
      floodFillSunlightAdd(nx - 1, ny, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny - 1, nz)) {
      floodFillSunlightAdd(nx, ny - 1, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz - 1)) {
      floodFillSunlightAdd(nx, ny, nz - 1, nextLightValue);
    }
  }
}

void World::floodFillSunlightAdd(uint16_t x, uint16_t y, uint16_t z,
                                 u8 nextLightValue) {
  auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));

  if (isTransparent(b)) {
    if (pLevel->GetSunLightFromMap(x, y, z) + 1 < nextLightValue) {
      addSunLight(x, y, z, nextLightValue);
    }
  }
}

void World::addSunLight(uint16_t x, uint16_t y, uint16_t z) {
  auto lightLevel = pLevel->GetSunLightFromMap(x, y, z);
  addSunLight(x, y, z, lightLevel);
}

void World::addSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  if (lightLevel >= 0) {
    pLevel->SetSunLightInMap(x, y, z, lightLevel);
    sunlightBfsQueue.emplace(x, y, z, lightLevel);
  }
}

void World::propagateSunlightRemovalQueue() {
  while (!sunlightRemovalBfsQueue.empty()) {
    // get the light value
    Node lightNode = sunlightRemovalBfsQueue.front();

    // get the index
    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    sunlightRemovalBfsQueue.pop();

    if (pLevel->BoundCheckMap(nx + 1, ny, nz)) {
      floodFillSunlightRemove(nx + 1, ny, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny + 1, nz)) {
      floodFillSunlightRemove(nx, ny + 1, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz + 1)) {
      floodFillSunlightRemove(nx, ny, nz + 1, lightValue);
    }

    if (pLevel->BoundCheckMap(nx - 1, ny, nz)) {
      floodFillSunlightRemove(nx - 1, ny, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny - 1, nz)) {
      floodFillSunlightRemove(nx, ny - 1, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz - 1)) {
      floodFillSunlightRemove(nx, ny, nz - 1, lightValue);
    }
  }
}

void World::floodFillSunlightRemove(uint16_t x, uint16_t y, uint16_t z,
                                    u8 lightLevel) {
  auto neighborLevel = pLevel->GetSunLightFromMap(x, y, z);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    removeSunLight(x, y, z);
  } else if (neighborLevel >= lightLevel) {
    addSunLight(x, y, z, neighborLevel);
  }
}

void World::removeSunLight(uint16_t x, uint16_t y, uint16_t z) {
  auto lightLevel = pLevel->GetSunLightFromMap(x, y, z);
  removeSunLight(x, y, z, lightLevel);
}

void World::removeSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  if (lightLevel > 0) {
    sunlightRemovalBfsQueue.emplace(x, y, z, lightLevel);
    pLevel->SetSunLightInMap(x, y, z, 0);
  }
}

void World::addBlockLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  if (lightLevel > 0) {
    lightBfsQueue.emplace(x, y, z, lightLevel);
    pLevel->SetBlockLightInMap(x, y, z, lightLevel);
  }
}

void World::removeLight(uint16_t x, uint16_t y, uint16_t z) {
  u8 lightLevel = pLevel->GetBlockLightFromMap(x, y, z);
  removeLight(x, y, z, lightLevel);
}

void World::removeLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  lightRemovalBfsQueue.emplace(x, y, z, lightLevel);
  pLevel->SetBlockLightInMap(x, y, z, 0);
}

void World::updateBlockLights() {
  if (lightRemovalBfsQueue.empty() == false) {
    propagateLightRemovalQueue();
  }

  if (lightBfsQueue.empty() == false) {
    propagateLightAddQueue();
  }
}

void World::propagateLightRemovalQueue() {
  while (lightRemovalBfsQueue.empty() == false) {
    // get the light value
    Node lightNode = lightRemovalBfsQueue.front();

    // get the index
    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    lightRemovalBfsQueue.pop();

    if (pLevel->BoundCheckMap(nx + 1, ny, nz)) {
      floodFillLightRemove(nx + 1, ny, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny + 1, nz)) {
      floodFillLightRemove(nx, ny + 1, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz + 1)) {
      floodFillLightRemove(nx, ny, nz + 1, lightValue);
    }

    if (pLevel->BoundCheckMap(nx - 1, ny, nz)) {
      floodFillLightRemove(nx - 1, ny, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny - 1, nz)) {
      floodFillLightRemove(nx, ny - 1, nz, lightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz - 1)) {
      floodFillLightRemove(nx, ny, nz - 1, lightValue);
    }
  }
}

void World::floodFillLightRemove(uint16_t x, uint16_t y, uint16_t z,
                                 u8 lightLevel) {
  auto neighborLevel = pLevel->GetBlockLightFromMap(x, y, z);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    removeLight(x, y, z);
  } else if (neighborLevel >= lightLevel) {
    addBlockLight(x, y, z, neighborLevel);
  }
}

void World::propagateLightAddQueue() {
  while (!lightBfsQueue.empty()) {
    auto lightNode = lightBfsQueue.front();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    lightBfsQueue.pop();

    s16 nextLightValue = lightValue - 1;

    if (nextLightValue <= 0) {
      continue;
    }

    if (pLevel->BoundCheckMap(nx + 1, ny, nz)) {
      floodFillLightAdd(nx + 1, ny, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny + 1, nz)) {
      floodFillLightAdd(nx, ny + 1, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz + 1)) {
      floodFillLightAdd(nx, ny, nz + 1, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx - 1, ny, nz)) {
      floodFillLightAdd(nx - 1, ny, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny - 1, nz)) {
      floodFillLightAdd(nx, ny - 1, nz, nextLightValue);
    }

    if (pLevel->BoundCheckMap(nx, ny, nz - 1)) {
      floodFillLightAdd(nx, ny, nz - 1, nextLightValue);
    }
  }
}

void World::floodFillLightAdd(uint16_t x, uint16_t y, uint16_t z,
                              u8 nextLightValue) {
  auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));
  if (isTransparent(b)) {
    if (pLevel->GetBlockLightFromMap(x, y, z) < nextLightValue) {
      addBlockLight(x, y, z, nextLightValue);
    }
  }
}

void World::checkSunLightAt(uint16_t x, uint16_t y, uint16_t z) {
  removeSunLight(x + 1, y, z);
  removeSunLight(x - 1, y, z);
  removeSunLight(x, y + 1, z);
  removeSunLight(x, y - 1, z);
  removeSunLight(x, y, z + 1);
  removeSunLight(x, y, z - 1);
  removeSunLight(x, y, z);

  return;
}

void World::initSunLight(uint32_t tick) {
  TYRA_LOG("Initiating SunLight...");

  for (int x = 0; x < pLevel->map.length; x++) {
    for (int z = 0; z < pLevel->map.width; z++) {
      u8 lv = 4;
      auto isDay = tick >= 0 && tick <= 12000;
      if (isDay) {
        lv = 15;
      }

      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(pLevel->GetBlockFromMap(x, y, z));
        // TODO: refactor to getLightFilterByBlock function
        // Vegetation
        // (b >= 37 && b <= 40)
        // Liquids
        // || (b >= 8 && b <= 11)
        if (b == Blocks::OAK_LEAVES_BLOCK) {
          if (lv >= 1)
            lv -= 1;
          else
            lv = 0;
        } else if (b == Blocks::WATER_BLOCK) {
          if (lv >= 2)
            lv -= 2;
          else
            lv = 0;
        } else if ((u8)b >= (u8)Blocks::STONE_SLAB &&
                   (u8)b <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
          if (lv >= 1)
            lv -= 1;
          else
            lv = 0;
        } else if (b != Blocks::AIR_BLOCK && b != Blocks::GLASS_BLOCK &&
                   b != Blocks::POPPY_FLOWER && b != Blocks::DANDELION_FLOWER &&
                   b != Blocks::GRASS) {
          lv = 0;
        }

        pLevel->SetSunLightInMap(x, y, z, lv);
        sunlightBfsQueue.emplace(x, y, z, lv);
        // printf("X: %d, Y: %d, Z: %d | b: %d | lv: %d \n", x, y, z, (u8)b,
        // lv);
      }
    }
  }
}

void World::initBlockLight(BlockManager* blockManager) {
  TYRA_LOG("Initiating block Lights...");

  for (int x = 0; x < pLevel->map.length; x++) {
    for (int z = 0; z < pLevel->map.width; z++) {
      for (int y = pLevel->map.height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(pLevel->SafeGetBlockFromMap(x, y, z));
        auto lightValue = blockManager->getBlockLightValue(b);
        if (lightValue > 0) {
          addBlockLight(x, y, z, lightValue);
        }
      }
    }
  }
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
