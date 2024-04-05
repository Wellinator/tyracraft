
#include "entities/World.hpp"
#include "entities/entity.hpp"
#include "3libs/bvh/bvh.h"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "managers/model_builder.hpp"
#include "managers/collision_manager.hpp"
#include "managers/mesh/mesh_builder.hpp"
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

World::World(const NewGameOptions& options) {
  seed = options.seed;

  printf("\n\n|-----------SEED---------|");
  printf("\n|           %li         |\n", seed);
  printf("|------------------------|\n\n");

  worldOptions = options;
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

void World::init(Renderer* renderer, ItemRepository* itemRepository,
                 SoundManager* t_soundManager) {
  // Set renders
  t_renderer = renderer;
  stapip.setRenderer(&t_renderer->core);

  // Set soundManager ref
  this->t_soundManager = t_soundManager;

  // Init light stuff
  dayNightCycleManager.init(t_renderer);
  initWorldLightModel();

  terrain = CrossCraft_World_GetMapPtr();
  blockManager.init(t_renderer, worldOptions.texturePack);
  chunckManager.init(&worldLightModel, terrain);
  cloudsManager.init(t_renderer, &worldLightModel);
  particlesManager.init(t_renderer, blockManager.getBlocksTexture(),
                        worldOptions.texturePack);
  mobManager.init(t_renderer, t_soundManager, &worldLightModel, terrain,
                  &chunckManager);
};

void World::generate() { CrossCraft_World_GenerateMap(worldOptions.type); }

void World::generateLight() {
  dayNightCycleManager.preLoad();
  updateLightModel();

  initSunLight(g_ticksCounter);
  initBlockLight(&blockManager);

  updateSunlight();
  updateBlockLights();
  chunckManager.reloadLightDataOfAllChunks();
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

void World::update(Player* t_player, Camera* t_camera, const float deltaTime) {
  dispatchChunkBatch();

  particlesManager.update(deltaTime, t_camera);
  cloudsManager.update(deltaTime);

  dispatchChunkBatch();

  dayNightCycleManager.update(deltaTime, &t_camera->position);
  if (affectedChunksIdByLiquidPropagation.size() > 0)
    updateChunksAffectedByLiquidPropagation();
  chunckManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll());

  dispatchChunkBatch();

  mobManager.update(deltaTime);
  updateTargetBlock(t_camera, t_player);
};

// Tick based logics
void World::tick(Player* t_player, Camera* t_camera) {
  particlesManager.tick();
  chunckManager.tick();
  mobManager.tick();
  cloudsManager.tick();
  dayNightCycleManager.tick();

  if (targetBlock && targetBlock->damage > 0) updateBlockDamage();

  // Update chunk light data every 1000 ticks
  if (isTicksCounterAt(1000)) {
    // TODO: refactor to event system
    updateLightModel();
    updateSunlight();
    updateBlockLights();
    chunckManager.enqueueChunksToReloadLight();
  }

  if (isTicksCounterAt(WATER_PROPAGATION_PER_TICKS)) {
    updateLiquidWater();
  }

  if (isTicksCounterAt(LAVA_PROPAGATION_PER_TICKS)) {
    updateLiquidLava();
  }

  // Update scheduled data every 4 ticks
  if (isTicksCounterAt(5)) {
    updateChunkByPlayerPosition(t_player);
  }

  t_renderer->core.setClearScreenColor(dayNightCycleManager.getSkyColor());
}

void World::renderOpaque() {
  chunckManager.rendererOpaque(t_renderer, &stapip, &blockManager);
};

void World::renderTransparent() {
  chunckManager.rendererTransparent(t_renderer, &stapip, &blockManager);
};

void World::buildInitialPosition() {
  TYRA_LOG("building initial position...");
  Chunck* initialChunck =
      chunckManager.getChunckByWorldPosition(worldSpawnArea);
  if (initialChunck != nullptr) {
    initialChunck->clear();
    buildChunk(initialChunck);
    scheduleChunksNeighbors(initialChunck, lastPlayerPosition, true);
  }
};

void World::resetWorldData() { chunckManager.clearAllChunks(); }

void World::updateChunkByPlayerPosition(Player* t_player) {
  Vec4 currentPlayerPos = *t_player->getPosition();
  float displacement =
      (lastPlayerPosition - currentPlayerPos).length() / CHUNCK_SIZE;

  if (displacement > CHUNCK_SIZE) {
    lastPlayerPosition.set(currentPlayerPos);
    Chunck* currentChunck =
        chunckManager.getChunckByWorldPosition(currentPlayerPos);

    if (currentChunck && t_player->currentChunckId != currentChunck->id) {
      t_player->currentChunckId = currentChunck->id;
      scheduleChunksNeighbors(currentChunck, currentPlayerPos);
    }
  }
}

void World::reloadWorldArea(const Vec4& position) {
  Chunck* currentChunck = chunckManager.getChunckByWorldPosition(position);
  if (currentChunck) {
    buildChunk(currentChunck);
    scheduleChunksNeighbors(currentChunck, position, true);
  }
}

void World::removeBlockFromChunk(Block* blockToRemove) {
  Chunck* currentChunk = chunckManager.getChunkById(blockToRemove->chunkId);
  if (!currentChunk) return;

  Vec4 offset;
  GetXYZFromPos(&blockToRemove->offset, &offset);

  currentChunk->removeBlock(blockToRemove);
  rebuildChunkFragment(currentChunk, &offset);
}

void World::updateNeighBorsChunksByAddedBlock(Vec4* offset) {
  Chunck* currentChunk = chunckManager.getChunkByBlockOffset(*offset);
  if (!currentChunk) return;

  addBlockToChunk(currentChunk, offset);
  rebuildChunkFragment(currentChunk, offset);
}

void World::scheduleChunksNeighbors(Chunck* origin_chunk,
                                    const Vec4 currentPlayerPos,
                                    u8 force_loading) {
  if (!canBuildChunk()) return;

  auto chuncks = chunckManager.getChuncks();
  for (u16 i = 0; i < chuncks->size(); i++) {
    auto t_chunk = (*chuncks)[i];
    const auto distance =
        floor(origin_chunk->center.distanceTo(t_chunk->center) / CHUNCK_SIZE) +
        1;

    if (distance > worldOptions.drawDistance) {
      if (force_loading) {
        t_chunk->clear();
      } else if (t_chunk->state != ChunkState::Clean) {
        addChunkToUnloadAsync(t_chunk);
      }

      t_chunk->setDistanceFromPlayerInChunks(-1);
    } else {
      if (force_loading) {
        t_chunk->clear();
        buildChunk(t_chunk);
      } else if (t_chunk->state == ChunkState::Clean) {
        addChunkToLoadAsync(t_chunk);
      }

      t_chunk->setDistanceFromPlayerInChunks(distance);
    }
  }

  if (!force_loading && !tempChuncksToLoad.empty())
    sortChunksToLoad(currentPlayerPos);
}

void World::sortChunksToLoad(const Vec4& currentPlayerPos) {
  std::sort(tempChuncksToLoad.begin(), tempChuncksToLoad.end(),
            [currentPlayerPos](const Chunck* a, const Chunck* b) {
              auto distanceA =
                  ((a->center * DUBLE_BLOCK_SIZE) - currentPlayerPos).length();
              auto distanceB =
                  ((b->center * DUBLE_BLOCK_SIZE) - currentPlayerPos).length();

              return distanceA < distanceB;
            });
}

void World::loadScheduledChunks() {
  if (tempChuncksToLoad.size() > 0 && canBuildChunk()) {
    Chunck* chunk = tempChuncksToLoad.front();
    if (chunk->state == ChunkState::PreLoaded) {
      if (!chunk->isDrawDataLoaded()) return chunk->loadDrawDataAsync();

      chunk->state = ChunkState::Loaded;

      if (g_debug_mode) {
        chunk->timeToBuild =
            ((float)(clock() - chunk->buildingTimeStart) / CLOCKS_PER_SEC);
        printf("Time to async build chunk %i: %f\n", chunk->id,
               chunk->timeToBuild);
      }

      // TODO: implement callback 'afterLoadChunk'
      const u8 shouldSpawnMobInChunk = Utils::Probability(0.0075f);

      // TODO: move to chunk randon tick
      // const u8 isInSpawnZone = chunk->getDistanceFromPlayerInChunks() <= 2;

      if (shouldSpawnMobInChunk) {
        Vec4 _spawnPosition;
        if (getOptimalSpawnPositionInChunk(chunk, &_spawnPosition)) {
          mobManager.spawnMobAtPosition(MobType::Pig, _spawnPosition);
        }
      }

      tempChuncksToLoad.pop_front();
      if (tempChuncksToLoad.size() == 0) {
        tempChuncksToLoad.shrink_to_fit();
      }
    } else if (chunk->state != ChunkState::Loaded) {
      return buildChunkAsync(chunk);
    }
  }
}

void World::unloadScheduledChunks() {
  if (tempChuncksToUnLoad.size() > 0) {
    Chunck* chunk = tempChuncksToUnLoad.front();

    if (chunk->state != ChunkState::Clean) {
      return chunk->clearAsync();
    } else {
      tempChuncksToUnLoad.pop_front();
      if (tempChuncksToUnLoad.size() == 0) {
        tempChuncksToUnLoad.shrink_to_fit();
      }
    }
  }
}

void World::renderBlockDamageOverlay() {
  if (!targetBlock) return;

  t_renderer->renderer3D.usePipeline(stapip);

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  textureBag.coordinates = _targetBlockUVMap.data();

  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  colorBag.many = _targetBlockColors.data();

  bag.count = _targetBlockVertices.size();
  bag.vertices = _targetBlockVertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  textureBag.texture = blockManager.getBlocksTexture();

  M4x4 model = M4x4::Identity;
  // model.scale(0.01f);
  infoBag.model = &model;

  stapip.core.render(&bag);
}

void World::addChunkToLoadAsync(Chunck* t_chunck) {
  // Avoid being duplicated;
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  t_chunck->state = ChunkState::Loading;
  tempChuncksToLoad.push_front(t_chunck);
}

void World::addChunkToUnloadAsync(Chunck* t_chunck) {
  // Avoid being duplicated;
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id)
      tempChuncksToLoad.erase(tempChuncksToLoad.begin() + i);

  tempChuncksToUnLoad.push_front(t_chunck);
}

void World::updateLightModel() {
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.sunLightIntensity =
      dayNightCycleManager.getSunLightIntensity();
}

u8 World::isAirAtPosition(const u8 x, const u8 y, const u8 z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    return GetBlockFromMap(terrain, x, y, z) == (u8)Blocks::AIR_BLOCK;
  }
  return false;
}

u8 World::isLiquidAtPosition(const u8 x, const u8 y, const u8 z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    const auto b = GetBlockFromMap(terrain, x, y, z);
    return b == (u8)Blocks::WATER_BLOCK || b == (u8)Blocks::LAVA_BLOCK;
  }
  return false;
}

u8 World::isBlockTransparentAtPosition(const u8 x, const u8 y, const u8 z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    const Blocks blockType =
        static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z));

    return (isTransparent(blockType) || blockType == Blocks::LAVA_BLOCK);
  } else {
    return false;
  }
}

u8 World::isTopFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y + 1,
                                      t_blockOffset->z);
}

u8 World::isBottomFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y - 1,
                                      t_blockOffset->z);
}

u8 World::isFrontFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z - 1);
}

u8 World::isBackFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z + 1);
}

u8 World::isLeftFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x + 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

u8 World::isRightFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x - 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

u8 World::getBlockVisibleFaces(const Vec4* t_blockOffset) {
  const BlockOrientation orientation = GetBlockOrientationDataFromMap(
      terrain, t_blockOffset->x, t_blockOffset->y, t_blockOffset->z);

  u8 result = 0b000000;

  switch (orientation) {
    case BlockOrientation::North:
      // Will be rotated by 90deg
      // Left turns Back & Right turns Front
      if (isLeftFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isFrontFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      break;
    case BlockOrientation::South:
      // Will be rotated by 270deg
      // Left turns Front & Right turns Back
      if (isLeftFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      if (isFrontFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      break;
    case BlockOrientation::West:
      // Will be rotated by 180deg
      // Left turns Right & Front turns Back
      if (isLeftFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isFrontFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      break;
    case BlockOrientation::East:
    default:
      if (isFrontFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isLeftFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      break;
  }

  if (isTopFaceVisible(t_blockOffset)) result = result | TOP_VISIBLE;
  if (isBottomFaceVisible(t_blockOffset)) result = result | BOTTOM_VISIBLE;

  return result;
}

u8 World::getSlabVisibleFaces(const Vec4* t_blockOffset) {
  const BlockOrientation orientationXZ = GetBlockOrientationDataFromMap(
      terrain, t_blockOffset->x, t_blockOffset->y, t_blockOffset->z);
  const SlabOrientation orientationY = GetSlabOrientationDataFromMap(
      terrain, t_blockOffset->x, t_blockOffset->y, t_blockOffset->z);

  u8 result = 0b000000;

  switch (orientationXZ) {
    case BlockOrientation::North:
      // Will be rotated by 90deg
      // Left turns Back & Right turns Front
      if (isLeftFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isFrontFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      break;
    case BlockOrientation::South:
      // Will be rotated by 270deg
      // Left turns Front & Right turns Back
      if (isLeftFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      if (isFrontFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      break;
    case BlockOrientation::West:
      // Will be rotated by 180deg
      // Left turns Right & Front turns Back
      if (isLeftFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isFrontFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      break;
    case BlockOrientation::East:
    default:
      if (isFrontFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;
      if (isBackFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;
      if (isRightFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;
      if (isLeftFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;
      break;
  }

  if (orientationY == SlabOrientation::Top) {
    if (isTopFaceVisible(t_blockOffset)) result = result | TOP_VISIBLE;
    result = result | BOTTOM_VISIBLE;
  } else if (orientationY == SlabOrientation::Bottom) {
    result = result | TOP_VISIBLE;
    if (isBottomFaceVisible(t_blockOffset)) result = result | BOTTOM_VISIBLE;
  }

  return result;
}

u8 World::getLeavesVisibleFaces(const Vec4* t_blockOffset) {
  u8 result = 0b000000;

  const auto x = t_blockOffset->x;
  const auto y = t_blockOffset->y;
  const auto z = t_blockOffset->z;

  const auto bFront =
      !BoundCheckMap(terrain, x, y, z - 1)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z - 1));

  const auto bBlack =
      !BoundCheckMap(terrain, x, y, z + 1)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z + 1));

  const auto bRight =
      !BoundCheckMap(terrain, x - 1, y, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x - 1, y, z));

  const auto bLeft =
      !BoundCheckMap(terrain, x + 1, y, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x + 1, y, z));

  const auto bTop =
      !BoundCheckMap(terrain, x, y + 1, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y + 1, z));

  const auto bBottom =
      !BoundCheckMap(terrain, x, y - 1, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y - 1, z));

  // Front
  if (!(bFront == Blocks::OAK_LEAVES_BLOCK ||
        bFront == Blocks::BIRCH_LEAVES_BLOCK) &&
      (Blocks::AIR_BLOCK == bFront || blockManager.isBlockTransparent(bFront)))
    result = result | FRONT_VISIBLE;
  // Back
  if (!(bBlack == Blocks::OAK_LEAVES_BLOCK ||
        bBlack == Blocks::BIRCH_LEAVES_BLOCK) &&
      (Blocks::AIR_BLOCK == bBlack || blockManager.isBlockTransparent(bBlack)))
    result = result | BACK_VISIBLE;
  // Right
  if (!(bRight == Blocks::OAK_LEAVES_BLOCK ||
        bRight == Blocks::BIRCH_LEAVES_BLOCK) &&
      (Blocks::AIR_BLOCK == bRight || blockManager.isBlockTransparent(bRight)))
    result = result | RIGHT_VISIBLE;
  // Left
  if (!(bLeft == Blocks::OAK_LEAVES_BLOCK ||
        bLeft == Blocks::BIRCH_LEAVES_BLOCK) &&
      (Blocks::AIR_BLOCK == bLeft || blockManager.isBlockTransparent(bLeft)))
    result = result | LEFT_VISIBLE;
  // Top
  if (!(bTop == Blocks::OAK_LEAVES_BLOCK ||
        bTop == Blocks::BIRCH_LEAVES_BLOCK) &&
      (Blocks::AIR_BLOCK == bTop || blockManager.isBlockTransparent(bTop)))
    result = result | TOP_VISIBLE;
  // Bottom
  if (!(bBottom == Blocks::OAK_LEAVES_BLOCK ||
        bBottom == Blocks::BIRCH_LEAVES_BLOCK) &&
      (Blocks::AIR_BLOCK == bBottom ||
       blockManager.isBlockTransparent(bBottom)))
    result = result | BOTTOM_VISIBLE;

  return result;
}

u8 World::getLiquidBlockVisibleFaces(const Vec4* t_blockOffset) {
  u8 result = 0b000000;

  const auto x = t_blockOffset->x;
  const auto y = t_blockOffset->y;
  const auto z = t_blockOffset->z;

  const u8 currentLevel = GetLiquidDataFromMap(terrain, x, y, z);

  const auto bFront =
      !BoundCheckMap(terrain, x, y, z - 1)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z - 1));

  const auto bBlack =
      !BoundCheckMap(terrain, x, y, z + 1)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z + 1));

  const auto bRight =
      !BoundCheckMap(terrain, x - 1, y, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x - 1, y, z));

  const auto bLeft =
      !BoundCheckMap(terrain, x + 1, y, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x + 1, y, z));

  const auto bTop =
      !BoundCheckMap(terrain, x, y + 1, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y + 1, z));

  const auto bBottom =
      !BoundCheckMap(terrain, x, y - 1, z)
          ? Blocks::VOID
          : static_cast<Blocks>(GetBlockFromMap(terrain, x, y - 1, z));

  // Front
  if (bFront != Blocks::LAVA_BLOCK && bFront != Blocks::WATER_BLOCK &&
      bFront != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bFront || blockManager.isBlockTransparent(bFront)))
    result = result | FRONT_VISIBLE;
  // Back
  if (bBlack != Blocks::LAVA_BLOCK && bBlack != Blocks::WATER_BLOCK &&
      bBlack != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bBlack || blockManager.isBlockTransparent(bBlack)))
    result = result | BACK_VISIBLE;
  // Right
  if (bRight != Blocks::LAVA_BLOCK && bRight != Blocks::WATER_BLOCK &&
      bRight != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bRight || blockManager.isBlockTransparent(bRight)))
    result = result | RIGHT_VISIBLE;
  // Left
  if (bLeft != Blocks::LAVA_BLOCK && bLeft != Blocks::WATER_BLOCK &&
      bLeft != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bLeft || blockManager.isBlockTransparent(bLeft)))
    result = result | LEFT_VISIBLE;

  // Top
  if (bTop != Blocks::LAVA_BLOCK && bTop != Blocks::WATER_BLOCK &&
      bTop != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bTop || blockManager.isBlockTransparent(bTop) ||
       currentLevel < (u8)LiquidLevel::Percent100))
    result = result | TOP_VISIBLE;
  // Bottom
  if (bBottom != Blocks::LAVA_BLOCK && bBottom != Blocks::WATER_BLOCK &&
      bBottom != Blocks::VOID &&
      (Blocks::AIR_BLOCK == bBottom ||
       blockManager.isBlockTransparent(bBottom)))
    result = result | BOTTOM_VISIBLE;

  return result;
}

const Vec4 World::defineSpawnArea() {
  Vec4 spawPos = calcSpawOffset();

  auto level = CrossCraft_World_GetLevelPtr();

  level->map.spawnX = spawPos.x;
  level->map.spawnY = spawPos.y;
  level->map.spawnZ = spawPos.z;

  return spawPos;
}

const Vec4 World::calcSpawOffset(int bias) {
  bool found = false;
  u8 airBlockCounter = 0;
  // Pick a X and Z coordinates based on the seed;
  int posX = ((seed + bias) % HALF_OVERWORLD_H_DISTANCE);
  int posZ = ((seed - bias) % HALF_OVERWORLD_H_DISTANCE);
  Vec4 result;

  for (int posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--) {
    u8 type = GetBlockFromMap(terrain, posX, posY, posZ);
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
    return result * DUBLE_BLOCK_SIZE;
  else
    return calcSpawOffset(bias + 1);
}

const bool World::calcSpawOffsetOfChunk(Vec4* result, const Vec4& minOffset,
                                        const Vec4& maxOffset, uint16_t bias) {
  bool found = false;
  u8 airBlockCounter = 0;
  // Pick a X and Z coordinates based on the min offset;
  int posX = minOffset.x + bias;
  int posZ = minOffset.z + bias;
  Vec4 tempResult;

  for (int posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--) {
    u8 type = GetBlockFromMap(terrain, posX, posY, posZ);

    if (type == (u8)Blocks::GRASS_BLOCK &&
        GetSunLightFromMap(terrain, posX, posY + 1, posZ) == 15 &&
        airBlockCounter >= 3) {
      found = true;
      tempResult.set(posX, posY + 1, posZ);
      break;
    }

    if (type == (u8)Blocks::AIR_BLOCK)
      airBlockCounter++;
    else
      airBlockCounter = 0;
  }

  if (found) {
    result->set(tempResult * DUBLE_BLOCK_SIZE);
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

const bool World::getOptimalSpawnPositionInChunk(const Chunck* targetChunk,
                                                 Vec4* result) {
  return calcSpawOffsetOfChunk(result, targetChunk->minOffset,
                               targetChunk->maxOffset);
}

void World::removeBlock(Block* blockToRemove) {
  // Generate amount of particles right begore block gets destroyed
  particlesManager.createBlockParticleBatch(blockToRemove, 18);

  Vec4 offsetToRemove;
  GetXYZFromPos(&blockToRemove->offset, &offsetToRemove);
  SetBlockInMapByIndex(terrain, blockToRemove->index, (u8)Blocks::AIR_BLOCK);
  SetLiquidDataToMap(terrain, offsetToRemove.x, offsetToRemove.y,
                     offsetToRemove.z, (u8)LiquidLevel::Percent0);

  // Update sunlight and block light at position
  removeLight(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);
  checkSunLightAt(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);
  updateSunlight();
  updateBlockLights();
  chunckManager.reloadLightData();

  // Update liquid at position
  checkLiquidPropagation(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);

  removeBlockFromChunk(blockToRemove);
  playDestroyBlockSound(blockToRemove->type);

  // Remove up block if it's is vegetation
  const Vec4 upBlockOffset =
      Vec4(offsetToRemove.x, offsetToRemove.y + 1, offsetToRemove.z);

  if (BoundCheckMap(terrain, upBlockOffset.x, upBlockOffset.y,
                    upBlockOffset.z)) {
    const Blocks b = static_cast<Blocks>(GetBlockFromMap(
        terrain, upBlockOffset.x, upBlockOffset.y, upBlockOffset.z));

    if (isVegetation(b) || b == Blocks::TORCH) {
      auto chunk = chunckManager.getChunkByBlockOffset(upBlockOffset);
      Block* upperBlock = chunk->getBlockByOffset(&upBlockOffset);
      if (upperBlock) removeBlock(upperBlock);
    }
  }
}

void World::putBlock(const Blocks& blockToPlace, Player* t_player,
                     const float cameraYaw) {
  Vec4 targetPos = ray.at(targetBlock->distance);

  PlacementDirection placementDirection = PlacementDirection::Top;

  Vec4 blockOffset;
  GetXYZFromPos(&targetBlock->offset, &blockOffset);

  // TODO: move to function
  // Front
  if (std::round(targetPos.z) ==
      std::round(targetBlock->bbox->getFrontFace().axisPosition)) {
    placementDirection = PlacementDirection::Front;
    blockOffset.z++;
    // Back
  } else if (std::round(targetPos.z) ==
             std::round(targetBlock->bbox->getBackFace().axisPosition)) {
    placementDirection = PlacementDirection::Back;
    blockOffset.z--;
    // Right
  } else if (std::round(targetPos.x) ==
             std::round(targetBlock->bbox->getRightFace().axisPosition)) {
    placementDirection = PlacementDirection::Right;
    blockOffset.x++;
    // Left
  } else if (std::round(targetPos.x) ==
             std::round(targetBlock->bbox->getLeftFace().axisPosition)) {
    placementDirection = PlacementDirection::Left;
    blockOffset.x--;
    // Up
  } else if (std::round(targetPos.y) ==
             std::round(targetBlock->bbox->getTopFace().axisPosition)) {
    placementDirection = PlacementDirection::Top;
    blockOffset.y++;
    // Down
  } else if (std::round(targetPos.y) ==
             std::round(targetBlock->bbox->getBottomFace().axisPosition)) {
    placementDirection = PlacementDirection::Bottom;
    blockOffset.y--;
  }

  // Placing block at invalid position
  if (!BoundCheckMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z))
    return;

  t_player->playPutBlockAnimation();

  switch (blockToPlace) {
    case Blocks::TORCH:
      putTorchBlock(placementDirection, cameraYaw, blockOffset);
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
      putSlab(blockToPlace, placementDirection, t_player, cameraYaw,
              blockOffset, targetPos);
      break;

    default:
      putDefaultBlock(blockToPlace, t_player, cameraYaw, blockOffset);
      break;
  }

  playPutBlockSound(blockToPlace);
  updateNeighBorsChunksByAddedBlock(&blockOffset);
}

void World::putTorchBlock(const PlacementDirection placementDirection,
                          const float cameraYaw, Vec4 blockOffset) {
  const Blocks blockTypeAtNewPosition = static_cast<Blocks>(
      GetBlockFromMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z));

  const u8 canReplace = blockTypeAtNewPosition == Blocks::AIR_BLOCK;

  if (targetBlock->type == Blocks::TORCH &&
      placementDirection == PlacementDirection::Top) {
    return;
  }

  if (canReplace) {
    // Calc block orientation
    BlockOrientation orientation;

    if (targetBlock->type == Blocks::TORCH) {
      orientation = BlockOrientation::Top;
    } else {
      // Torch orientation must be reverse of placement direction
      switch (placementDirection) {
        case PlacementDirection::Top:
          orientation = BlockOrientation::Top;
          break;
        case PlacementDirection::Left:
          orientation = BlockOrientation::East;
          break;
        case PlacementDirection::Right:
          orientation = BlockOrientation::West;
          break;
        case PlacementDirection::Front:
          orientation = BlockOrientation::North;
          break;
        case PlacementDirection::Back:
          orientation = BlockOrientation::South;
          break;

        case PlacementDirection::Bottom:
        default:
          return;
      }
    }

    SetBlockInMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z,
                  static_cast<u8>(Blocks::TORCH));
    SetTorchOrientationDataToMap(terrain, blockOffset.x, blockOffset.y,
                                 blockOffset.z, orientation);
    checkSunLightAt(blockOffset.x, blockOffset.y, blockOffset.z);

    const auto lightValue = blockManager.getBlockLightValue(Blocks::TORCH);
    addBlockLight(blockOffset.x, blockOffset.y, blockOffset.z, lightValue);

    updateSunlight();
    updateBlockLights();

    chunckManager.reloadLightData();
  }
}

void World::putSlab(const Blocks& blockType,
                    const PlacementDirection placementDirection,
                    Player* t_player, const float cameraYaw, Vec4 blockOffset,
                    Vec4 targetPos) {
  Vec4 originalOffset;
  GetXYZFromPos(&targetBlock->offset, &originalOffset);

  const Blocks blockTypeAtTargetPosition = static_cast<Blocks>(GetBlockFromMap(
      terrain, originalOffset.x, originalOffset.y, originalOffset.z));

  // Is placing two slabs at target position?
  // If so, fix the blockOffset;
  if ((u8)blockTypeAtTargetPosition >= (u8)Blocks::STONE_SLAB &&
      (u8)blockTypeAtTargetPosition <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
    const auto targetSlabHeightOrientation = GetSlabOrientationDataFromMap(
        terrain, originalOffset.x, originalOffset.y, originalOffset.z);

    if (targetSlabHeightOrientation == SlabOrientation::Bottom &&
        blockOffset.y > originalOffset.y) {
      blockOffset.y--;
    } else if (targetSlabHeightOrientation == SlabOrientation::Top &&
               blockOffset.y < originalOffset.y) {
      blockOffset.y++;
    }
  }

  const Blocks blockTypeAtOffsetPosition = static_cast<Blocks>(
      GetBlockFromMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z));

  // Is placing two slabs at offset position?
  if ((u8)blockTypeAtOffsetPosition >= (u8)Blocks::STONE_SLAB &&
      (u8)blockTypeAtOffsetPosition <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
    // If so, check if are same slab type;
    // Prevent to place a different type of slab;
    if (blockTypeAtOffsetPosition != blockType) {
      return;
    } else {
      // convert double slab to solid block
      Blocks newBlock;

      switch (blockType) {
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

      ResetSlabOrientationDataToMap(terrain, blockOffset.x, blockOffset.y,
                                    blockOffset.z);

      // Calc block orientation
      BlockOrientation orientation;

      if (blockManager.isBlockOriented(blockType)) {
        if (cameraYaw > 315 || cameraYaw < 45) {
          orientation = BlockOrientation::North;
        } else if (cameraYaw >= 135 && cameraYaw <= 225) {
          orientation = BlockOrientation::South;
        } else if (cameraYaw >= 45 && cameraYaw <= 135) {
          orientation = BlockOrientation::East;
        } else {
          orientation = BlockOrientation::West;
        }
      } else {
        orientation = BlockOrientation::East;
      }
      SetBlockOrientationDataToMap(terrain, blockOffset.x, blockOffset.y,
                                   blockOffset.z, orientation);
      SetBlockInMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z,
                    static_cast<u8>(Blocks::AIR_BLOCK));
      putDefaultBlock(newBlock, t_player, cameraYaw, blockOffset);
      return;
    }
  }

  // Prevent to put a block at the player position;
  Vec4 newBlockPos = blockOffset * DUBLE_BLOCK_SIZE;
  M4x4 tempModel = M4x4();
  tempModel.identity();
  tempModel.scaleX(BLOCK_SIZE);
  tempModel.scaleZ(BLOCK_SIZE);
  tempModel.scaleY(HALF_BLOCK_SIZE);
  tempModel.translate(newBlockPos);

  BBox* rawBBox = VertexBlockData::getRawBBoxByBlock(
      blockType, GetPosFromXYZ(blockOffset.x, blockOffset.y, blockOffset.z));
  BBox tempBBox = rawBBox->getTransformed(tempModel);
  BBox finalBBox = BBox(tempBBox.vertices, tempBBox.getVertexCount());

  Vec4 newBlockPosMin;
  Vec4 newBlockPosMax;
  finalBBox.getMinMax(&newBlockPosMin, &newBlockPosMax);

  Vec4 minPlayerCorner;
  Vec4 maxPlayerCorner;
  t_player->getHitBox().getMinMax(&minPlayerCorner, &maxPlayerCorner);

  delete rawBBox;

  // Will Collide to player?
  if (newBlockPosMax.x > minPlayerCorner.x &&
      newBlockPosMin.x < maxPlayerCorner.x &&
      newBlockPosMax.z > minPlayerCorner.z &&
      newBlockPosMin.z < maxPlayerCorner.z &&
      newBlockPosMax.y > minPlayerCorner.y &&
      newBlockPosMin.y < maxPlayerCorner.y) {
    return;  // Return on collision
  }

  const float heightOffset =
      std::fmod(targetPos.y - BLOCK_SIZE, DUBLE_BLOCK_SIZE);

  const SlabOrientation slabOrientation = heightOffset > BLOCK_SIZE
                                              ? SlabOrientation::Top
                                              : SlabOrientation::Bottom;
  SetSlabOrientationDataToMap(terrain, blockOffset.x, blockOffset.y,
                              blockOffset.z, slabOrientation);

  const u8 canReplace = blockTypeAtOffsetPosition == Blocks::AIR_BLOCK ||
                        blockTypeAtOffsetPosition == Blocks::WATER_BLOCK ||
                        blockTypeAtOffsetPosition == Blocks::LAVA_BLOCK ||
                        blockTypeAtOffsetPosition == Blocks::TORCH ||
                        blockTypeAtOffsetPosition == Blocks::POPPY_FLOWER ||
                        blockTypeAtOffsetPosition == Blocks::DANDELION_FLOWER ||
                        blockTypeAtOffsetPosition == Blocks::GRASS;

  if (canReplace) {
    SetBlockInMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z,
                  static_cast<u8>(blockType));
    checkSunLightAt(blockOffset.x, blockOffset.y, blockOffset.z);
    removeLight(blockOffset.x, blockOffset.y, blockOffset.z);

    updateSunlight();
    updateBlockLights();

    chunckManager.reloadLightData();

    const Blocks oldTypeBlock = blockTypeAtOffsetPosition;

    // It was liquid at position
    if (oldTypeBlock == Blocks::WATER_BLOCK ||
        oldTypeBlock == Blocks::LAVA_BLOCK) {
      removeLiquid(blockOffset.x, blockOffset.y, blockOffset.z,
                   (u8)oldTypeBlock);
    }
  }
}

void World::putDefaultBlock(const Blocks blockToPlace, Player* t_player,
                            const float cameraYaw, Vec4 blockOffset) {
  Vec4 newBlockPos = blockOffset * DUBLE_BLOCK_SIZE;

  // Prevent to put a block at the player position;
  M4x4 tempModel = M4x4();
  tempModel.identity();
  tempModel.scale(BLOCK_SIZE);
  tempModel.translate(newBlockPos);

  BBox* rawBBox = VertexBlockData::getRawBBoxByBlock(
      blockToPlace, GetPosFromXYZ(blockOffset.x, blockOffset.y, blockOffset.z));
  BBox tempBBox = rawBBox->getTransformed(tempModel);
  BBox finalBBox = BBox(tempBBox.vertices, tempBBox.getVertexCount());

  Vec4 newBlockPosMin;
  Vec4 newBlockPosMax;
  finalBBox.getMinMax(&newBlockPosMin, &newBlockPosMax);

  Vec4 minPlayerCorner;
  Vec4 maxPlayerCorner;
  t_player->getHitBox().getMinMax(&minPlayerCorner, &maxPlayerCorner);

  delete rawBBox;

  // Will Collide to player?
  if (newBlockPosMax.x > minPlayerCorner.x &&
      newBlockPosMin.x < maxPlayerCorner.x &&
      newBlockPosMax.z > minPlayerCorner.z &&
      newBlockPosMin.z < maxPlayerCorner.z &&
      newBlockPosMax.y > minPlayerCorner.y &&
      newBlockPosMin.y < maxPlayerCorner.y) {
    return;  // Return on collision
  }

  const Blocks blockTypeAtTargetPosition = static_cast<Blocks>(
      GetBlockFromMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z));
  const Blocks oldTypeBlock = blockTypeAtTargetPosition;

  const u8 canReplace = blockTypeAtTargetPosition == Blocks::WATER_BLOCK ||
                        blockTypeAtTargetPosition == Blocks::LAVA_BLOCK ||
                        blockTypeAtTargetPosition == Blocks::AIR_BLOCK ||
                        blockTypeAtTargetPosition == Blocks::TORCH ||
                        blockTypeAtTargetPosition == Blocks::POPPY_FLOWER ||
                        blockTypeAtTargetPosition == Blocks::DANDELION_FLOWER ||
                        blockTypeAtTargetPosition == Blocks::GRASS;

  if (canReplace) {
    // Calc block orientation
    BlockOrientation orientation;

    if (blockManager.isBlockOriented(blockToPlace)) {
      if (cameraYaw > 315 || cameraYaw < 45) {
        orientation = BlockOrientation::North;
      } else if (cameraYaw >= 135 && cameraYaw <= 225) {
        orientation = BlockOrientation::South;
      } else if (cameraYaw >= 45 && cameraYaw <= 135) {
        orientation = BlockOrientation::East;
      } else {
        orientation = BlockOrientation::West;
      }
    } else {
      orientation = BlockOrientation::East;
    }

    SetBlockInMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z,
                  static_cast<u8>(blockToPlace));
    SetBlockOrientationDataToMap(terrain, blockOffset.x, blockOffset.y,
                                 blockOffset.z, orientation);
    checkSunLightAt(blockOffset.x, blockOffset.y, blockOffset.z);

    const auto lightValue = blockManager.getBlockLightValue(blockToPlace);
    if (lightValue > 0) {
      addBlockLight(blockOffset.x, blockOffset.y, blockOffset.z, lightValue);
    } else {
      removeLight(blockOffset.x, blockOffset.y, blockOffset.z);
    }

    updateSunlight();
    updateBlockLights();

    chunckManager.reloadLightData();

    const u8 isPlacingLiquid = blockToPlace == Blocks::WATER_BLOCK ||
                               blockToPlace == Blocks::LAVA_BLOCK;
    const u8 itWasLiquidAtPosition = oldTypeBlock == Blocks::WATER_BLOCK ||
                                     oldTypeBlock == Blocks::LAVA_BLOCK;

    if (isPlacingLiquid) {
      addLiquid(blockOffset.x, blockOffset.y, blockOffset.z, (u8)blockToPlace,
                (u8)LiquidLevel::Percent100, (u8)orientation);
    } else if (itWasLiquidAtPosition) {
      removeLiquid(blockOffset.x, blockOffset.y, blockOffset.z,
                   (u8)oldTypeBlock);
    }
  }
}

void World::stopBreakTargetBlock() {
  _isBreakingBlock = false;
  breaking_time_pessed = 0;
  if (targetBlock) {
    targetBlock->damage = 0;
    clearTargetBlockDrawData();
    buildTargetBlockDrawData();
  }
}

void World::breakTargetBlock(const float& deltaTime) {
  if (targetBlock == nullptr) return;

  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;
    const auto breakingTime = blockManager.getBlockBreakingTime(targetBlock);
    if (breaking_time_pessed >= breakingTime) {
      // Remove block;
      removeBlock(targetBlock);

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
        playBreakingBlockSound(targetBlock->type);
        lastTimePlayedBreakingSfx = 0;
      } else {
        lastTimePlayedBreakingSfx += deltaTime;
      }
    }
  } else {
    breaking_time_pessed = 0;
    _isBreakingBlock = true;
  }
}

void World::breakTargetBlockInCreativeMode(const float& deltaTime) {
  if (targetBlock == nullptr) return;

  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;
    const auto breakingTime = BREAKING_TIME_IN_CREATIVE_MODE;
    if (breaking_time_pessed >= breakingTime) {
      // Remove block;
      removeBlock(targetBlock);

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
        playBreakingBlockSound(targetBlock->type);
        lastTimePlayedBreakingSfx = 0;
      } else {
        lastTimePlayedBreakingSfx += deltaTime;
      }
    }
  } else {
    breaking_time_pessed = 0;
    _isBreakingBlock = true;
  }
}

void World::playPutBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getDigSoundByBlockType(blockType);
    if (blockSfxModel) {
      const int ch = t_soundManager->getAvailableChannel();
      SfxLibrarySound* sound = t_soundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getPlaceSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      t_soundManager->setSfxVolume(config->_volume, ch);
      t_soundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void World::playDestroyBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getBrokenSoundByBlockType(blockType);

    if (blockSfxModel) {
      const int ch = t_soundManager->getAvailableChannel();
      SfxLibrarySound* sound = t_soundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getBrokenSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      t_soundManager->setSfxVolume(config->_volume, ch);
      t_soundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void World::playBreakingBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getDigSoundByBlockType(blockType);

    if (blockSfxModel) {
      const int ch = t_soundManager->getAvailableChannel();
      SfxLibrarySound* sound = t_soundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getBreakingSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      t_soundManager->setSfxVolume(config->_volume, ch);
      t_soundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void World::playFlowingWaterSound() {
  SfxBlockModel waterSfxModel = SfxBlockModel(
      Blocks::WATER_BLOCK, SoundFxCategory::Liquid, SoundFX::Water);

  const int ch = t_soundManager->getAvailableChannel();
  SfxLibrarySound* sound = t_soundManager->getSound(&waterSfxModel);

  const u8 pitch = Tyra::Math::randomi(50, 150);
  const u8 volume = Tyra::Math::randomi(75, 100);

  sound->_sound->pitch = pitch;
  t_soundManager->setSfxVolume(volume, ch);
  t_soundManager->playSfx(sound, ch);
}

u8 World::isCrossedBlock(Blocks block_type) {
  return block_type == Blocks::POPPY_FLOWER ||
         block_type == Blocks::DANDELION_FLOWER || block_type == Blocks::GRASS;
}

void World::rebuildChunkFragment(Chunck* t_chunck, Vec4* moddedOffset) {
  if (g_debug_mode) t_chunck->buildingTimeStart = clock();

  std::vector<Chunck*> affected_chunks;
  affected_chunks.emplace_back(t_chunck);

  Vec4 bottom = *moddedOffset + Vec4(0, -1, 0);
  if (t_chunck->containsBlock(&bottom)) {
    addOrupdateBlockInChunk(t_chunck, &bottom);
  } else if (t_chunck->bottomNeighbor) {
    addOrupdateBlockInChunk(t_chunck->bottomNeighbor, &bottom);
    affected_chunks.emplace_back(t_chunck->bottomNeighbor);
  }

  Vec4 top = *moddedOffset + Vec4(0, 1, 0);
  if (t_chunck->containsBlock(&top)) {
    addOrupdateBlockInChunk(t_chunck, &top);
  } else if (t_chunck->topNeighbor) {
    addOrupdateBlockInChunk(t_chunck->topNeighbor, &top);
    affected_chunks.emplace_back(t_chunck->topNeighbor);
  }

  Vec4 right = *moddedOffset + Vec4(-1, 0, 0);
  if (t_chunck->containsBlock(&right)) {
    addOrupdateBlockInChunk(t_chunck, &right);
  } else if (t_chunck->rightNeighbor) {
    addOrupdateBlockInChunk(t_chunck->rightNeighbor, &right);
    affected_chunks.emplace_back(t_chunck->rightNeighbor);
  }

  Vec4 left = *moddedOffset + Vec4(1, 0, 0);
  if (t_chunck->containsBlock(&left)) {
    addOrupdateBlockInChunk(t_chunck, &left);
  } else if (t_chunck->leftNeighbor) {
    addOrupdateBlockInChunk(t_chunck->leftNeighbor, &left);
    affected_chunks.emplace_back(t_chunck->leftNeighbor);
  }

  Vec4 front = *moddedOffset + Vec4(0, 0, -1);
  if (t_chunck->containsBlock(&front)) {
    addOrupdateBlockInChunk(t_chunck, &front);
  } else if (t_chunck->frontNeighbor) {
    addOrupdateBlockInChunk(t_chunck->frontNeighbor, &front);
    affected_chunks.emplace_back(t_chunck->frontNeighbor);
  }

  Vec4 back = *moddedOffset + Vec4(0, 0, 1);
  if (t_chunck->containsBlock(&back)) {
    addOrupdateBlockInChunk(t_chunck, &back);
  } else if (t_chunck->backNeighbor) {
    addOrupdateBlockInChunk(t_chunck->backNeighbor, &back);
    affected_chunks.emplace_back(t_chunck->backNeighbor);
  }

  for (size_t i = 0; i < affected_chunks.size(); i++) {
    affected_chunks[i]->clearDrawData();
    affected_chunks[i]->loadDrawData();
  }

  if (g_debug_mode) {
    t_chunck->timeToBuild =
        ((float)(clock() - t_chunck->buildingTimeStart)) / CLOCKS_PER_SEC;
    printf("Time to sync build chunk fragment %i: %f\n", t_chunck->id,
           t_chunck->timeToBuild);
  }
}

void World::addOrupdateBlockInChunk(Chunck* t_chunck, Vec4* moddedOffset) {
  const u8 validOffset =
      BoundCheckMap(terrain, moddedOffset->x, moddedOffset->y, moddedOffset->z);
  const u8 isInChunk =
      moddedOffset->collidesBox(t_chunck->minOffset, t_chunck->maxOffset);

  if (validOffset && isInChunk) {
    Block* t_block = t_chunck->getBlockByOffset(moddedOffset);

    if (t_block) {
      // TODO: check if need to remove block
      // refactore to updateOrRemoveBlockInChunk
      updateOrRemoveBlockInChunk(t_chunck, t_block);
    } else {
      addBlockToChunk(t_chunck, moddedOffset);
    }
  }
}

void World::updateOrRemoveBlockInChunk(Chunck* t_chunck, Block* t_block) {
  Vec4 tempBlockOffset;
  GetXYZFromPos(&t_block->offset, &tempBlockOffset);

  const Blocks block_type = static_cast<Blocks>(t_block->type);
  u8 visibleFaces;

  if (block_type == Blocks::WATER_BLOCK || block_type == Blocks::LAVA_BLOCK) {
    visibleFaces = getLiquidBlockVisibleFaces(&tempBlockOffset);
  } else if ((u8)block_type >= (u8)Blocks::STONE_SLAB &&
             (u8)block_type <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
    visibleFaces = getSlabVisibleFaces(&tempBlockOffset);
  } else if (block_type == Blocks::OAK_LEAVES_BLOCK ||
             block_type == Blocks::BIRCH_LEAVES_BLOCK) {
    visibleFaces = getLeavesVisibleFaces(&tempBlockOffset);
  } else {
    visibleFaces = getBlockVisibleFaces(&tempBlockOffset);
  }

  // Check if visible faces changed
  if (visibleFaces != t_block->visibleFaces) {
    t_block->visibleFaces = visibleFaces;
  }
}

void World::addBlockToChunk(Chunck* t_chunck, Vec4* offset) {
  u32 blockIndex = getIndexByOffset(offset->x, offset->y, offset->z);

  const Blocks block_type = static_cast<Blocks>(terrain->blocks[blockIndex]);

  if (block_type != Blocks::AIR_BLOCK) {
    u8 visibleFaces;

    if (block_type == Blocks::WATER_BLOCK || block_type == Blocks::LAVA_BLOCK) {
      visibleFaces = getLiquidBlockVisibleFaces(offset);
    } else if ((u8)block_type >= (u8)Blocks::STONE_SLAB &&
               (u8)block_type <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
      visibleFaces = getSlabVisibleFaces(offset);
    } else if (block_type == Blocks::OAK_LEAVES_BLOCK ||
               block_type == Blocks::BIRCH_LEAVES_BLOCK) {
      visibleFaces = getLeavesVisibleFaces(offset);
    } else {
      visibleFaces = getBlockVisibleFaces(offset);
    }

    // Have any face visible?
    if (visibleFaces > 0) {
      BlockInfo* blockInfo = blockManager.getBlockInfoByType(block_type);

      if (blockInfo) {
        Block* block = new Block(blockInfo);
        block->index = blockIndex;
        block->offset = GetPosFromXYZ(offset->x, offset->y, offset->z);
        block->chunkId = t_chunck->id;

        if (block->isCrossed) {
          block->visibleFaces = 0b111111;
          block->visibleFacesCount = 2;
        } else {
          block->visibleFaces = visibleFaces;
          block->visibleFacesCount = Utils::countSetBits(visibleFaces);
        }

        block->position.set((*offset) * DUBLE_BLOCK_SIZE);

        ModelBuilder_BuildModel(block, terrain);
        BBox* rawBBox =
            VertexBlockData::getRawBBoxByBlock(block->type, block->offset);
        BBox tempBBox = rawBBox->getTransformed(block->model);

        block->bbox = new BBox(tempBBox.vertices, tempBBox.getVertexCount());
        block->bbox->getMinMax(&block->minCorner, &block->maxCorner);

        // Add data to AABBTree
        bvh::AABB blockAABB = bvh::AABB();
        blockAABB.minx = block->minCorner.x;
        blockAABB.miny = block->minCorner.y;
        blockAABB.minz = block->minCorner.z;
        blockAABB.maxx = block->maxCorner.x;
        blockAABB.maxy = block->maxCorner.y;
        blockAABB.maxz = block->maxCorner.z;
        block->tree_index = g_AABBTree->insert(blockAABB, block);

        delete rawBBox;

        t_chunck->addBlock(block);
      }
    }
  }
}

// TODO: move to chunk builder
void World::buildChunk(Chunck* t_chunck) {
  if (!canBuildChunk()) return;
  if (g_debug_mode) t_chunck->buildingTimeStart = clock();

  t_chunck->preAllocateMemory();

  for (size_t x = t_chunck->minOffset.x; x < t_chunck->maxOffset.x; x++) {
    for (size_t z = t_chunck->minOffset.z; z < t_chunck->maxOffset.z; z++) {
      for (size_t y = t_chunck->minOffset.y; y < t_chunck->maxOffset.y; y++) {
        Vec4 tempBlockOffset = Vec4(x, y, z);
        addBlockToChunk(t_chunck, &tempBlockOffset);
      }
    }
  }

  t_chunck->freeUnusedMemory();
  t_chunck->state = ChunkState::Loaded;

  if (g_debug_mode) {
    t_chunck->timeToBuild =
        ((float)(clock() - t_chunck->buildingTimeStart)) / CLOCKS_PER_SEC;
    printf("Time to sync build chunk %i: %f\n", t_chunck->id,
           t_chunck->timeToBuild);
  }
}

void World::buildChunkAsync(Chunck* t_chunck) {
  if (g_debug_mode) {
    if (t_chunck->state == ChunkState::Clean) {
      t_chunck->buildingTimeStart = clock();
    }
  }

  uint16_t batchCounter = 0;
  uint16_t x = t_chunck->tempLoadingOffset.x;
  uint16_t y = t_chunck->tempLoadingOffset.y;
  uint16_t z = t_chunck->tempLoadingOffset.z;

  if (!t_chunck->isPreAllocated()) t_chunck->preAllocateMemory();

  while (batchCounter < LOAD_CHUNK_BATCH) {
    t_chunck->state = ChunkState::Loading;

    if (x >= t_chunck->maxOffset.x) break;

    u32 blockIndex = getIndexByOffset(x, y, z);
    const Blocks block_type = static_cast<Blocks>(terrain->blocks[blockIndex]);

    if (block_type != Blocks::AIR_BLOCK) {
      Vec4 tempBlockOffset = Vec4(x, y, z);
      u8 visibleFaces;

      if (block_type == Blocks::WATER_BLOCK ||
          block_type == Blocks::LAVA_BLOCK) {
        visibleFaces = getLiquidBlockVisibleFaces(&tempBlockOffset);
      } else if ((u8)block_type >= (u8)Blocks::STONE_SLAB &&
                 (u8)block_type <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB) {
        visibleFaces = getSlabVisibleFaces(&tempBlockOffset);
      } else if (block_type == Blocks::OAK_LEAVES_BLOCK ||
                 block_type == Blocks::BIRCH_LEAVES_BLOCK) {
        visibleFaces = getLeavesVisibleFaces(&tempBlockOffset);
      } else {
        visibleFaces = getBlockVisibleFaces(&tempBlockOffset);
      }

      // Is any face vísible?
      if (visibleFaces > 0) {
        batchCounter++;

        BlockInfo* blockInfo = blockManager.getBlockInfoByType(block_type);

        if (blockInfo) {
          Block* block = new Block(blockInfo);
          block->index = blockIndex;
          block->offset = GetPosFromXYZ(tempBlockOffset.x, tempBlockOffset.y,
                                        tempBlockOffset.z);
          block->chunkId = t_chunck->id;

          if (block->isCrossed) {
            block->visibleFaces = 0b111111;
            block->visibleFacesCount = 2;
          } else {
            block->visibleFaces = visibleFaces;
            block->visibleFacesCount = Utils::countSetBits(visibleFaces);
          }

          block->position.set(tempBlockOffset * DUBLE_BLOCK_SIZE);

          ModelBuilder_BuildModel(block, terrain);

          BBox* rawBBox =
              VertexBlockData::getRawBBoxByBlock(block->type, block->offset);
          BBox tempBBox = rawBBox->getTransformed(block->model);

          block->bbox = new BBox(tempBBox.vertices, tempBBox.getVertexCount());
          block->bbox->getMinMax(&block->minCorner, &block->maxCorner);

          delete rawBBox;

          // Add data to AABBTree
          bvh::AABB blockAABB = bvh::AABB();
          blockAABB.minx = block->minCorner.x;
          blockAABB.miny = block->minCorner.y;
          blockAABB.minz = block->minCorner.z;
          blockAABB.maxx = block->maxCorner.x;
          blockAABB.maxy = block->maxCorner.y;
          blockAABB.maxz = block->maxCorner.z;
          block->tree_index = g_AABBTree->insert(blockAABB, block);

          t_chunck->addBlock(block);
        }
      }
    }

    y++;
    if (y >= t_chunck->maxOffset.y) {
      y = t_chunck->minOffset.y;
      z++;
    }
    if (z >= t_chunck->maxOffset.z) {
      z = t_chunck->minOffset.z;
      x++;
    }
  }

  if (batchCounter >= LOAD_CHUNK_BATCH) {
    t_chunck->tempLoadingOffset.set(x, y, z);
    return;
  }

  t_chunck->state = ChunkState::PreLoaded;
  t_chunck->freeUnusedMemory();
}

void World::updateTargetBlock(Camera* t_camera, Player* t_player) {
  const Vec4 baseOrigin =
      *t_player->getPosition() + Vec4(0.0f, t_camera->getCamY(), 0.0f);
  bool hitedABlock = false;
  float tempTargetDistance = -1.0f;
  float tempPlayerDistance = -1.0f;
  Block* tempTargetBlock = nullptr;
  uint32_t _lastTargetBlockId = 0;

  if (targetBlock) {
    _lastTargetBlockId = targetBlock->index;
  }

  // Reset the current target block;
  targetBlock = nullptr;

  // Prepate the raycast
  ray.origin.set(baseOrigin);
  ray.direction.set(t_camera->unitCirclePosition.getNormalized());

  std::vector<index_t> ni;
  g_AABBTree->intersectLine(ray.origin, ray.at(MAX_RANGE_PICKER), ni);

  for (u16 b = 0; b < ni.size(); b++) {
    Entity* entity = (Entity*)g_AABBTree->user_data(ni[b]);
    if (entity->entity_type == EntityType::Block) {
      Block* block = (Block*)entity;

      if (!block->isBreakable) continue;

      float distanceFromCurrentBlockToPlayer =
          baseOrigin.distanceTo(entity->position);

      // Reset block state
      block->isTarget = false;
      block->distance = -1.0f;

      float intersectionPoint;
      if (ray.intersectBox(entity->minCorner, entity->maxCorner,
                           &intersectionPoint)) {
        hitedABlock = true;
        if (tempTargetDistance == -1.0f ||
            (distanceFromCurrentBlockToPlayer < tempPlayerDistance)) {
          tempTargetBlock = block;
          tempTargetDistance = intersectionPoint;
          tempPlayerDistance = distanceFromCurrentBlockToPlayer;
        }
      }
    }
  }

  if (hitedABlock) {
    targetBlock = tempTargetBlock;
    targetBlock->isTarget = true;
    targetBlock->distance = tempTargetDistance;
    targetBlock->hitPosition.set(ray.at(tempTargetDistance));

    if (targetBlock->index != _lastTargetBlockId) {
      breaking_time_pessed = 0;
      clearTargetBlockDrawData();
      buildTargetBlockDrawData();
    };
  } else {
    if (_targetBlockVertices.size() > 0) clearTargetBlockDrawData();
  }
}

void World::buildTargetBlockDrawData() {
  TYRA_ASSERT(targetBlock != nullptr, "No target block to build draw data!");

  const u8 size = targetBlock->visibleFacesCount * VertexBlockData::FACES_COUNT;

  _targetBlockVertices.reserve(size);
  _targetBlockColors.reserve(size);
  _targetBlockUVMap.reserve(size);

  MeshBuilder_BuildMesh(targetBlock, &_targetBlockVertices, &_targetBlockColors,
                        &_targetBlockUVMap, &worldLightModel, terrain);

  const float highLight = 25.0f;
  for (size_t i = 0; i < size; i++) _targetBlockColors[i] += highLight;
}

void World::updateBlockDamage() {
  _targetBlockUVMap.clear();

  const u8 V = 15;
  const u8 U = floor(targetBlock->damage / 10);
  const float _scale = 1.0F / 16.0F;
  const Vec4 UVScale = Vec4(_scale, _scale, 1.0F, 0.0F);
  const u8 size = targetBlock->visibleFacesCount * VertexBlockData::FACES_COUNT;

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
  _targetBlockVertices.shrink_to_fit();

  _targetBlockColors.clear();
  _targetBlockColors.shrink_to_fit();

  _targetBlockUVMap.clear();
  _targetBlockUVMap.shrink_to_fit();
}

void World::setDrawDistace(const u8& drawDistanceInChunks) {
  if (drawDistanceInChunks >= MIN_DRAW_DISTANCE &&
      drawDistanceInChunks <= MAX_DRAW_DISTANCE) {
    worldOptions.drawDistance = drawDistanceInChunks;
    Chunck* currentChunck =
        chunckManager.getChunckByWorldPosition(lastPlayerPosition);
    TYRA_ASSERT(currentChunck, "Invalid chunk pointer");
    if (currentChunck) {
      scheduleChunksNeighbors(currentChunck, lastPlayerPosition, true);
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
  if (BoundCheckMap(terrain, x - 1, y, z)) {
    Blocks nl = static_cast<Blocks>(GetBlockFromMap(terrain, x - 1, y, z));
    u8 level = GetLiquidDataFromMap(terrain, x - 1, y, z);

    if (nl == Blocks::WATER_BLOCK || nl == Blocks::LAVA_BLOCK) {
      addLiquid(x - 1, y, z, (u8)nl, level);
    }
  }

  if (BoundCheckMap(terrain, x + 1, y, z)) {
    Blocks nr = static_cast<Blocks>(GetBlockFromMap(terrain, x + 1, y, z));
    u8 level = GetLiquidDataFromMap(terrain, x + 1, y, z);

    if (nr == Blocks::WATER_BLOCK || nr == Blocks::LAVA_BLOCK) {
      addLiquid(x + 1, y, z, (u8)nr, level);
    }
  }

  if (BoundCheckMap(terrain, x, y - 1, z)) {
    Blocks nd = static_cast<Blocks>(GetBlockFromMap(terrain, x, y - 1, z));
    u8 level = GetLiquidDataFromMap(terrain, x, y - 1, z);

    if (nd == Blocks::WATER_BLOCK || nd == Blocks::LAVA_BLOCK) {
      addLiquid(x, y - 1, z, (u8)nd, level);
    }
  }

  if (BoundCheckMap(terrain, x, y, z + 1)) {
    Blocks nf = static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z + 1));
    u8 level = GetLiquidDataFromMap(terrain, x, y, z + 1);

    if (nf == Blocks::WATER_BLOCK || nf == Blocks::LAVA_BLOCK) {
      addLiquid(x, y, z + 1, (u8)nf, level);
    }
  }

  if (BoundCheckMap(terrain, x, y, z - 1)) {
    Blocks nb = static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z - 1));
    u8 level = GetLiquidDataFromMap(terrain, x, y, z - 1);

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

    SetBlockInMap(terrain, x, y, z, type);
    SetLiquidDataToMap(terrain, x, y, z, level);

    const auto prevDir = GetLiquidOrientationDataFromMap(terrain, x, y, z);

    // Fix traversal orientation to linear
    // It two traversal flux encounter each other, it will become a linear flux
    if (((u8)LiquidOrientation::NorthEast == orientation &&
         LiquidOrientation::NorthWest == prevDir) ||
        ((u8)LiquidOrientation::NorthWest == orientation &&
         LiquidOrientation::NorthEast == prevDir)) {
      SetLiquidOrientationDataToMap(terrain, x, y, z, LiquidOrientation::North);
    } else if (((u8)LiquidOrientation::NorthEast == orientation &&
                LiquidOrientation::SouthEast == prevDir) ||
               ((u8)LiquidOrientation::SouthEast == orientation &&
                LiquidOrientation::NorthEast == prevDir)) {
      SetLiquidOrientationDataToMap(terrain, x, y, z, LiquidOrientation::East);
    } else if (((u8)LiquidOrientation::SouthEast == orientation &&
                LiquidOrientation::SouthWest == prevDir) ||
               ((u8)LiquidOrientation::SouthWest == orientation &&
                LiquidOrientation::SouthEast == prevDir)) {
      SetLiquidOrientationDataToMap(terrain, x, y, z, LiquidOrientation::South);
    } else if (((u8)LiquidOrientation::NorthWest == orientation &&
                LiquidOrientation::SouthWest == prevDir) ||
               ((u8)LiquidOrientation::SouthWest == orientation &&
                LiquidOrientation::NorthWest == prevDir)) {
      SetLiquidOrientationDataToMap(terrain, x, y, z, LiquidOrientation::West);
    } else {
      SetLiquidOrientationDataToMap(
          terrain, x, y, z, static_cast<LiquidOrientation>(orientation));
    }

    Chunck* moddedChunk = chunckManager.getChunkByBlockOffset(Vec4(x, y, z));
    if (moddedChunk && moddedChunk->isDrawDataLoaded()) {
      affectedChunksIdByLiquidPropagation.insert(moddedChunk);
    }
  }
}

void World::addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level) {
  addLiquid(x, y, z, type, level, (u8)LiquidOrientation::East);
}

void World::removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type) {
  u8 liquidLevel = GetLiquidDataFromMap(terrain, x, y, z);
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

    SetLiquidDataToMap(terrain, x, y, z, level);
  } else {
    SetBlockInMap(terrain, x, y, z, (u8)Blocks::AIR_BLOCK);
    SetLiquidDataToMap(terrain, x, y, z, (u8)LiquidLevel::Percent0);
  }

  Chunck* moddedChunk = chunckManager.getChunkByBlockOffset(Vec4(x, y, z));
  if (moddedChunk) {
    affectedChunksIdByLiquidPropagation.insert(moddedChunk);
  }
}

void World::initLiquidExpansion() {
  TYRA_LOG("Initiating water propagation...");
  auto map = CrossCraft_World_GetMapPtr();

  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      for (int y = map->height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(GetBlockFromMap(map, x, y, z));

        if (b == Blocks::AIR_BLOCK) {
          auto liquidValue = LiquidLevel::Percent100;

          if (BoundCheckMap(terrain, x - 1, y, z)) {
            auto type = GetBlockFromMap(terrain, x - 1, y, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x - 1, y, z, type, liquidValue);
          } else if (BoundCheckMap(terrain, x + 1, y, z)) {
            auto type = GetBlockFromMap(terrain, x + 1, y, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x + 1, y, z, type, liquidValue);
          } else if (BoundCheckMap(terrain, x, y - 1, z)) {
            auto type = GetBlockFromMap(terrain, x, y - 1, z);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y - 1, z, type, liquidValue);
          } else if (BoundCheckMap(terrain, x, y, z - 1)) {
            auto type = GetBlockFromMap(terrain, x, y, z - 1);
            if (type == (u8)Blocks::WATER_BLOCK ||
                type == (u8)Blocks::LAVA_BLOCK)
              addLiquid(x, y, z - 1, type, liquidValue);
          } else if (BoundCheckMap(terrain, x, y, z + 1)) {
            auto type = GetBlockFromMap(terrain, x, y, z + 1);
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

      if (BoundCheckMap(terrain, nx + 1, ny, nz) &&
          GetBlockFromMap(terrain, nx + 1, ny, nz) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx + 1, ny, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (BoundCheckMap(terrain, nx, ny, nz + 1) &&
          GetBlockFromMap(terrain, nx, ny, nz + 1) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx, ny, nz + 1, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (BoundCheckMap(terrain, nx - 1, ny, nz) &&
          GetBlockFromMap(terrain, nx - 1, ny, nz) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx - 1, ny, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (BoundCheckMap(terrain, nx, ny - 1, nz) &&
          GetBlockFromMap(terrain, nx, ny - 1, nz) == (u8)Blocks::WATER_BLOCK) {
        floodFillLiquidRemove(nx, ny - 1, nz, (u8)Blocks::WATER_BLOCK,
                              liquidValue);
      }

      if (BoundCheckMap(terrain, nx, ny, nz - 1) &&
          GetBlockFromMap(terrain, nx, ny, nz - 1) == (u8)Blocks::WATER_BLOCK) {
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

      if (BoundCheckMap(terrain, nx + 1, ny, nz) &&
          GetBlockFromMap(terrain, nx + 1, ny, nz) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx + 1, ny, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (BoundCheckMap(terrain, nx, ny, nz + 1) &&
          GetBlockFromMap(terrain, nx, ny, nz + 1) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx, ny, nz + 1, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (BoundCheckMap(terrain, nx - 1, ny, nz) &&
          GetBlockFromMap(terrain, nx - 1, ny, nz) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx - 1, ny, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (BoundCheckMap(terrain, nx, ny - 1, nz) &&
          GetBlockFromMap(terrain, nx, ny - 1, nz) == (u8)Blocks::LAVA_BLOCK) {
        floodFillLiquidRemove(nx, ny - 1, nz, (u8)Blocks::LAVA_BLOCK,
                              nextLevel);
      }

      if (BoundCheckMap(terrain, nx, ny, nz - 1) &&
          GetBlockFromMap(terrain, nx, ny, nz - 1) == (u8)Blocks::LAVA_BLOCK) {
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
  u8 neighborLevel = GetLiquidDataFromMap(terrain, x, y, z);

  if (neighborLevel <= level + 1) {
    removeLiquid(x, y, z, type, level);
  } else if (neighborLevel > level) {
    addLiquid(x, y, z, type, neighborLevel);
  }
}

void World::floodFillLiquidAdd(uint16_t x, uint16_t y, uint16_t z, u8 type,
                               u8 nextLevel, u8 orientation) {
  if (GetLiquidDataFromMap(terrain, x, y, z) + 1 < nextLevel) {
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
  for (auto chunckPtr : affectedChunksIdByLiquidPropagation) {
    Chunck* moddedChunk = chunckPtr;
    if (moddedChunk) {
      moddedChunk->clear();
      buildChunk(moddedChunk);
    }
  }

  affectedChunksIdByLiquidPropagation.clear();
}

u8 World::canPropagateLiquid(uint16_t x, uint16_t y, uint16_t z) {
  if (!BoundCheckMap(terrain, x, y, z)) return false;
  const u8 type = GetBlockFromMap(terrain, x, y, z);
  return type == (u8)Blocks::AIR_BLOCK || type == (u8)Blocks::GRASS ||
         type == (u8)Blocks::POPPY_FLOWER || type == (u8)Blocks::TORCH ||
         type == (u8)Blocks::DANDELION_FLOWER;
}

// From CrossCraft
std::queue<Node> lightBfsQueue;
std::queue<Node> lightRemovalBfsQueue;

// std::stack<Node> sunlightBfsQueue;
std::queue<Node> sunlightBfsQueue;
std::queue<Node> sunlightRemovalBfsQueue;

void updateSunlight() {
  if (sunlightRemovalBfsQueue.empty() == false) {
    propagateSunlightRemovalQueue();
  }

  if (sunlightBfsQueue.empty() == false) {
    propagateSunLightAddBFSQueue();
  }
}

void propagateSunLightAddBFSQueue() {
  while (!sunlightBfsQueue.empty()) {
    auto lightNode = sunlightBfsQueue.front();
    auto map = CrossCraft_World_GetMapPtr();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    sunlightBfsQueue.pop();

    int nextLightValue = lightValue - 1;

    if (nextLightValue < 0) {
      continue;
    }

    if (BoundCheckMap(map, nx + 1, ny, nz)) {
      floodFillSunlightAdd(nx + 1, ny, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny + 1, nz)) {
      floodFillSunlightAdd(nx, ny + 1, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz + 1)) {
      floodFillSunlightAdd(nx, ny, nz + 1, nextLightValue);
    }

    if (BoundCheckMap(map, nx - 1, ny, nz)) {
      floodFillSunlightAdd(nx - 1, ny, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny - 1, nz)) {
      floodFillSunlightAdd(nx, ny - 1, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz - 1)) {
      floodFillSunlightAdd(nx, ny, nz - 1, nextLightValue);
    }
  }
}

void floodFillSunlightAdd(uint16_t x, uint16_t y, uint16_t z,
                          u8 nextLightValue) {
  auto map = CrossCraft_World_GetMapPtr();
  auto b = static_cast<Blocks>(GetBlockFromMap(map, x, y, z));

  if (isTransparent(b)) {
    if (GetSunLightFromMap(map, x, y, z) + 1 < nextLightValue) {
      addSunLight(x, y, z, nextLightValue);
    }
  }
}

void addSunLight(uint16_t x, uint16_t y, uint16_t z) {
  auto map = CrossCraft_World_GetMapPtr();
  auto lightLevel = GetSunLightFromMap(map, x, y, z);
  addSunLight(x, y, z, lightLevel);
}

void addSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  if (lightLevel >= 0) {
    SetSunLightInMap(CrossCraft_World_GetMapPtr(), x, y, z, lightLevel);
    sunlightBfsQueue.emplace(x, y, z, lightLevel);
  }
}

void propagateSunlightRemovalQueue() {
  while (!sunlightRemovalBfsQueue.empty()) {
    // get the light value
    Node lightNode = sunlightRemovalBfsQueue.front();
    auto map = CrossCraft_World_GetMapPtr();

    // get the index
    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    sunlightRemovalBfsQueue.pop();

    if (BoundCheckMap(map, nx + 1, ny, nz)) {
      floodFillSunlightRemove(nx + 1, ny, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny + 1, nz)) {
      floodFillSunlightRemove(nx, ny + 1, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz + 1)) {
      floodFillSunlightRemove(nx, ny, nz + 1, lightValue);
    }

    if (BoundCheckMap(map, nx - 1, ny, nz)) {
      floodFillSunlightRemove(nx - 1, ny, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny - 1, nz)) {
      floodFillSunlightRemove(nx, ny - 1, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz - 1)) {
      floodFillSunlightRemove(nx, ny, nz - 1, lightValue);
    }
  }
}

void floodFillSunlightRemove(uint16_t x, uint16_t y, uint16_t z,
                             u8 lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  auto neighborLevel = GetSunLightFromMap(map, x, y, z);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    removeSunLight(x, y, z);
  } else if (neighborLevel >= lightLevel) {
    addSunLight(x, y, z, neighborLevel);
  }
}

void removeSunLight(uint16_t x, uint16_t y, uint16_t z) {
  auto map = CrossCraft_World_GetMapPtr();
  auto lightLevel = GetSunLightFromMap(map, x, y, z);
  removeSunLight(x, y, z, lightLevel);
}

void removeSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  if (lightLevel > 0) {
    sunlightRemovalBfsQueue.emplace(x, y, z, lightLevel);
    SetSunLightInMap(CrossCraft_World_GetMapPtr(), x, y, z, 0);
  }
}

void addBlockLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  if (lightLevel > 0) {
    auto map = CrossCraft_World_GetMapPtr();
    lightBfsQueue.emplace(x, y, z, lightLevel);
    SetBlockLightInMap(map, x, y, z, lightLevel);
  }
}

void removeLight(uint16_t x, uint16_t y, uint16_t z) {
  auto map = CrossCraft_World_GetMapPtr();
  u8 lightLevel = GetBlockLightFromMap(map, x, y, z);
  removeLight(x, y, z, lightLevel);
}

void removeLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  lightRemovalBfsQueue.emplace(x, y, z, lightLevel);
  SetBlockLightInMap(map, x, y, z, 0);
}

void updateBlockLights() {
  if (lightRemovalBfsQueue.empty() == false) {
    propagateLightRemovalQueue();
  }

  if (lightBfsQueue.empty() == false) {
    propagateLightAddQueue();
  }
}

void propagateLightRemovalQueue() {
  while (lightRemovalBfsQueue.empty() == false) {
    // get the light value
    Node lightNode = lightRemovalBfsQueue.front();
    auto map = CrossCraft_World_GetMapPtr();

    // get the index
    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    lightRemovalBfsQueue.pop();

    if (BoundCheckMap(map, nx + 1, ny, nz)) {
      floodFillLightRemove(nx + 1, ny, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny + 1, nz)) {
      floodFillLightRemove(nx, ny + 1, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz + 1)) {
      floodFillLightRemove(nx, ny, nz + 1, lightValue);
    }

    if (BoundCheckMap(map, nx - 1, ny, nz)) {
      floodFillLightRemove(nx - 1, ny, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny - 1, nz)) {
      floodFillLightRemove(nx, ny - 1, nz, lightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz - 1)) {
      floodFillLightRemove(nx, ny, nz - 1, lightValue);
    }
  }
}

void floodFillLightRemove(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  auto neighborLevel = GetBlockLightFromMap(map, x, y, z);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    removeLight(x, y, z);
  } else if (neighborLevel >= lightLevel) {
    addBlockLight(x, y, z, neighborLevel);
  }
}

void propagateLightAddQueue() {
  while (!lightBfsQueue.empty()) {
    auto lightNode = lightBfsQueue.front();
    auto map = CrossCraft_World_GetMapPtr();

    uint16_t nx = lightNode.x;
    uint16_t ny = lightNode.y;
    uint16_t nz = lightNode.z;
    uint8_t lightValue = lightNode.val;

    lightBfsQueue.pop();

    s16 nextLightValue = lightValue - 1;

    if (nextLightValue <= 0) {
      continue;
    }

    if (BoundCheckMap(map, nx + 1, ny, nz)) {
      floodFillLightAdd(nx + 1, ny, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny + 1, nz)) {
      floodFillLightAdd(nx, ny + 1, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz + 1)) {
      floodFillLightAdd(nx, ny, nz + 1, nextLightValue);
    }

    if (BoundCheckMap(map, nx - 1, ny, nz)) {
      floodFillLightAdd(nx - 1, ny, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny - 1, nz)) {
      floodFillLightAdd(nx, ny - 1, nz, nextLightValue);
    }

    if (BoundCheckMap(map, nx, ny, nz - 1)) {
      floodFillLightAdd(nx, ny, nz - 1, nextLightValue);
    }
  }
}

void floodFillLightAdd(uint16_t x, uint16_t y, uint16_t z, u8 nextLightValue) {
  auto map = CrossCraft_World_GetMapPtr();
  auto b = static_cast<Blocks>(GetBlockFromMap(map, x, y, z));
  if (isTransparent(b)) {
    if (GetBlockLightFromMap(map, x, y, z) < nextLightValue) {
      addBlockLight(x, y, z, nextLightValue);
    }
  }
}

void checkSunLightAt(uint16_t x, uint16_t y, uint16_t z) {
  removeSunLight(x + 1, y, z);
  removeSunLight(x - 1, y, z);
  removeSunLight(x, y + 1, z);
  removeSunLight(x, y - 1, z);
  removeSunLight(x, y, z + 1);
  removeSunLight(x, y, z - 1);
  removeSunLight(x, y, z);

  return;
}

void initSunLight(uint32_t tick) {
  TYRA_LOG("Initiating SunLight...");
  auto map = CrossCraft_World_GetMapPtr();

  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      u8 lv = 4;
      auto isDay = tick >= 0 && tick <= 12000;
      if (isDay) {
        lv = 15;
      }

      for (int y = map->height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(GetBlockFromMap(map, x, y, z));
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

        SetSunLightInMap(map, x, y, z, lv);
        sunlightBfsQueue.emplace(x, y, z, lv);
        // printf("X: %d, Y: %d, Z: %d | b: %d | lv: %d \n", x, y, z, (u8)b,
        // lv);
      }
    }
  }
}

void initBlockLight(BlockManager* blockManager) {
  TYRA_LOG("Initiating block Lights...");
  auto map = CrossCraft_World_GetMapPtr();

  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      for (int y = map->height - 1; y >= 0; y--) {
        auto b = static_cast<Blocks>(GetBlockFromMap(map, x, y, z));
        auto lightValue = blockManager->getBlockLightValue(b);
        if (lightValue > 0) {
          addBlockLight(x, y, z, lightValue);
        }
      }
    }
  }
}

void CrossCraft_World_Init(const uint32_t& seed) {
  TYRA_LOG("Generating base level template");
  srand(seed);

  CrossCraft_WorldGenerator_Init(rand());

  const auto level = CrossCraft_World_GetLevelPtr();

  level->map = {
      .width = OVERWORLD_H_DISTANCE,
      .length = OVERWORLD_H_DISTANCE,
      .height = OVERWORLD_V_DISTANCE,

      .spawnX = 128,
      .spawnY = 59,
      .spawnZ = 128,
  };

  // For some reason I need to clear the array garbage
  // I was initialized with new key word, very weird!
  for (size_t i = 0; i < OVERWORLD_SIZE; i++) {
    level->map.blocks[i] = 0;
    level->map.lightData[i] = 0;
    level->map.metaData[i] = 0;
  }

  TYRA_LOG("Generated base level template");
}

void CrossCraft_World_Deinit() { TYRA_LOG("Destroying the world"); }

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void CrossCraft_World_GenerateMap(WorldType worldType) {
  const auto level = CrossCraft_World_GetLevelPtr();
  switch (worldType) {
    case WORLD_TYPE_ORIGINAL:
      CrossCraft_WorldGenerator_Generate_Original(&level->map);
      break;
    case WORLD_TYPE_FLAT:
      CrossCraft_WorldGenerator_Generate_Flat(&level->map);
      break;
    case WORLD_TYPE_ISLAND:
      CrossCraft_WorldGenerator_Generate_Island(&level->map);
      break;
    case WORLD_TYPE_WOODS:
      CrossCraft_WorldGenerator_Generate_Woods(&level->map);
      break;
    case WORLD_TYPE_FLOATING:
      CrossCraft_WorldGenerator_Generate_Floating(&level->map);
      break;
  }
}
