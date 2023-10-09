
#include "entities/World.hpp"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"
#include <tyra>

// From CrossCraft
#include <stdio.h>
#include "entities/World.hpp"
#include <queue>
#include <stack>

using Tyra::Color;
using Tyra::M4x4;

static Level level;

World::World(const NewGameOptions& options) {
  seed = options.seed;

  printf("\n\n|-----------SEED---------|");
  printf("\n|           %li         |\n", seed);
  printf("|------------------------|\n\n");

  worldOptions = options;
  setIntialTime();
  CrossCraft_World_Init(seed);
}

World::~World() {
  delete rawBlockBbox;
  CrossCraft_World_Deinit();
}

void World::init(Renderer* renderer, ItemRepository* itemRepository,
                 SoundManager* t_soundManager) {
  // Set renders
  t_renderer = renderer;
  mcPip.setRenderer(&t_renderer->core);
  stapip.setRenderer(&t_renderer->core);
  overlayData.reserve(1);

  // Set soundManager ref
  this->t_soundManager = t_soundManager;

  // Init light stuff
  dayNightCycleManager.init(t_renderer);
  initWorldLightModel();

  terrain = CrossCraft_World_GetMapPtr();
  blockManager.init(t_renderer, &mcPip, worldOptions.texturePack);
  chunckManager.init(&worldLightModel, terrain);
  cloudsManager.init(t_renderer, &worldLightModel);
  particlesManager.init(t_renderer, blockManager.getBlocksTexture(),
                        worldOptions.texturePack);

  calcRawBlockBBox(&mcPip);
};

void World::generate() { CrossCraft_World_GenerateMap(worldOptions.type); }

void World::generateLight() {
  dayNightCycleManager.update();
  updateLightModel();

  initSunLight(static_cast<uint32_t>(g_ticksCounter));
  initBlockLight(&blockManager);

  updateSunlight();
  updateBlockLights();
  chunckManager.reloadLightDataOfAllChunks();
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
  cloudsManager.update();

  particlesManager.update(deltaTime, t_camera);

  dayNightCycleManager.update(&t_camera->position);

  t_renderer->core.setClearScreenColor(dayNightCycleManager.getSkyColor());

  // Update chunk light data every 250 ticks
  if ((static_cast<uint32_t>(g_ticksCounter) % 1000) == 0) {
    // TODO: refactor to event system
    updateLightModel();
    updateSunlight();
    updateBlockLights();
    chunckManager.enqueueChunksToReloadLight();
  }

  chunckManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll(),
                       *t_player->getPosition());

  updateChunkByPlayerPosition(t_player);

  unloadScheduledChunks();
  loadScheduledChunks();

  updateTargetBlock(t_camera, t_player, chunckManager.getNearByChunks());
};

void World::render() {
  chunckManager.renderer(t_renderer, &stapip, &blockManager);

  if (targetBlock) {
    renderTargetBlockHitbox(targetBlock);
    if (isBreakingBLock() && targetBlock->damage > 0)
      renderBlockDamageOverlay();
  }
};

void World::buildInitialPosition() {
  Chunck* initialChunck = chunckManager.getChunckByPosition(worldSpawnArea);
  if (initialChunck != nullptr) {
    initialChunck->clear();
    buildChunk(initialChunck);
    scheduleChunksNeighbors(initialChunck, lastPlayerPosition, true);
    chunckManager.sortChunkByPlayerPosition(&lastPlayerPosition);
  }
};

void World::resetWorldData() { chunckManager.clearAllChunks(); }

void World::updateChunkByPlayerPosition(Player* t_player) {
  Vec4 currentPlayerPos = *t_player->getPosition();
  if (lastPlayerPosition.distanceTo(currentPlayerPos) > CHUNCK_SIZE) {
    lastPlayerPosition.set(currentPlayerPos);
    Chunck* currentChunck = chunckManager.getChunckByPosition(currentPlayerPos);

    if (currentChunck && t_player->currentChunckId != currentChunck->id) {
      chunckManager.sortChunkByPlayerPosition(&lastPlayerPosition);
      t_player->currentChunckId = currentChunck->id;
      scheduleChunksNeighbors(currentChunck, currentPlayerPos);
    }
  }
}

void World::reloadWorldArea(const Vec4& position) {
  Chunck* currentChunck = chunckManager.getChunckByPosition(position);
  if (currentChunck) {
    buildChunk(currentChunck);
    scheduleChunksNeighbors(currentChunck, position, true);
  }
}

void World::updateNeighBorsChunksByModdedPosition(const Vec4& pos) {
  Chunck* currentChunk = chunckManager.getChunckByOffset(pos);
  if (!currentChunk) return;

  const u8 isAtBorder = isBlockAtChunkBorder(&pos, currentChunk->minOffset,
                                             currentChunk->maxOffset);

  if (isAtBorder) {
    if (currentChunk->maxOffset->z - 1 == pos.z) {
      // Left
      Chunck* leftChunk =
          chunckManager.getChunckByOffset(Vec4(pos.x, pos.y, pos.z + 1));
      if (leftChunk && leftChunk->id != currentChunk->id) {
        leftChunk->clear();
        buildChunk(leftChunk);
      }
    } else if (currentChunk->minOffset->z == pos.z) {
      // Right
      Chunck* rightChunk =
          chunckManager.getChunckByOffset(Vec4(pos.x, pos.y, pos.z - 1));

      if (rightChunk && rightChunk->id != currentChunk->id) {
        rightChunk->clear();
        buildChunk(rightChunk);
      }
    }

    if (currentChunk->maxOffset->x - 1 == pos.x) {
      // Front
      Chunck* frontChunk =
          chunckManager.getChunckByOffset(Vec4(pos.x + 1, pos.y, pos.z));

      if (frontChunk && frontChunk->id != currentChunk->id) {
        frontChunk->clear();
        buildChunk(frontChunk);
      }
    } else if (currentChunk->minOffset->x == pos.x) {
      // Back
      Chunck* backChunk =
          chunckManager.getChunckByOffset(Vec4(pos.x - 1, pos.y, pos.z));
      if (backChunk && backChunk->id != currentChunk->id) {
        backChunk->clear();
        buildChunk(backChunk);
      }
    }

    if (currentChunk->maxOffset->y - 1 == pos.y) {
      // Top
      Chunck* topChunk =
          chunckManager.getChunckByOffset(Vec4(pos.x, pos.y + 1, pos.z));

      if (topChunk && topChunk->id != currentChunk->id) {
        topChunk->clear();
        buildChunk(topChunk);
      }
    } else if (currentChunk->minOffset->y == pos.y) {
      // Bottom
      Chunck* bottomChunk =
          chunckManager.getChunckByOffset(Vec4(pos.x, pos.y - 1, pos.z));

      if (bottomChunk && bottomChunk->id != currentChunk->id) {
        bottomChunk->clear();
        buildChunk(bottomChunk);
      }
    }
  }

  currentChunk->clear();
  buildChunk(currentChunk);
}

void World::scheduleChunksNeighbors(Chunck* t_chunck,
                                    const Vec4 currentPlayerPos,
                                    u8 force_loading) {
  const auto& chuncks = chunckManager.getChuncks();
  for (u16 i = 0; i < chuncks.size(); i++) {
    const auto distance =
        floor(t_chunck->center->distanceTo(*chuncks[i]->center) / CHUNCK_SIZE) +
        1;

    if (distance > worldOptions.drawDistance) {
      if (force_loading) {
        chuncks[i]->clear();
      } else if (chuncks[i]->state != ChunkState::Clean) {
        addChunkToUnloadAsync(chuncks[i]);
      }

      chuncks[i]->distanceFromPlayerInChunks = -1;
    } else {
      if (force_loading) {
        chuncks[i]->clear();
        buildChunk(chuncks[i]);
      } else if (chuncks[i]->state != ChunkState::Loaded) {
        addChunkToLoadAsync(chuncks[i]);
      }
      chuncks[i]->distanceFromPlayerInChunks = distance;
    }
  }

  if (tempChuncksToLoad.size()) sortChunksToLoad(currentPlayerPos);
}

void World::sortChunksToLoad(const Vec4& currentPlayerPos) {
  std::sort(tempChuncksToLoad.begin(), tempChuncksToLoad.end(),
            [currentPlayerPos](const Chunck* a, const Chunck* b) {
              const float distanceA =
                  (*a->center * DUBLE_BLOCK_SIZE).distanceTo(currentPlayerPos);
              const float distanceB =
                  (*b->center * DUBLE_BLOCK_SIZE).distanceTo(currentPlayerPos);
              return distanceA < distanceB;
            });
}

void World::loadScheduledChunks() {
  if (tempChuncksToLoad.size() > 0) {
    Chunck* chunk = tempChuncksToLoad.front();
    if (chunk->state == ChunkState::PreLoaded) {
      chunk->loadDrawData();
      tempChuncksToLoad.pop_front();
      return;
    } else if (chunk->state != ChunkState::Loaded) {
      return buildChunkAsync(chunk, worldOptions.drawDistance);
    }
    chunckManager.sortChunkByPlayerPosition(&lastPlayerPosition);
  }
}

void World::unloadScheduledChunks() {
  if (tempChuncksToUnLoad.size() > 0) {
    Chunck* chunk = tempChuncksToUnLoad.front();
    if (chunk->state != ChunkState::Clean) {
      chunk->clear();
      tempChuncksToUnLoad.pop_front();
      return;
    }
    chunckManager.sortChunkByPlayerPosition(&lastPlayerPosition);
  }
}

void World::renderBlockDamageOverlay() {
  McpipBlock* overlay = blockManager.getDamageOverlay(targetBlock->damage);

  if (overlayData.size() > 0) {
    // Clear last overlay;
    for (u8 i = 0; i < overlayData.size(); i++) {
      delete overlayData[i]->color;
      delete overlayData[i]->model;
    }
    overlayData.clear();
  }

  M4x4 scale = M4x4();
  M4x4 translation = M4x4();

  scale.identity();
  scale.scale(BLOCK_SIZE + 0.015f);

  translation.identity();
  translation.translate(targetBlock->position);

  overlay->model = new M4x4(translation * scale);
  overlay->color = new Color(128.0f, 128.0f, 128.0f, 70.0f);

  overlayData.emplace_back(overlay);
  t_renderer->renderer3D.usePipeline(&mcPip);
  mcPip.render(overlayData, blockManager.getBlocksTexture(), false);
}

void World::renderTargetBlockHitbox(Block* targetBlock) {
  t_renderer->renderer3D.utility.drawBox(targetBlock->position, BLOCK_SIZE,
                                         Color(0, 0, 0));
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

u32 World::getIndexByOffset(int x, int y, int z) {
  return (y * terrain->length * terrain->width) + (z * terrain->width) + x;
}

bool World::isAirAtPosition(const float& x, const float& y, const float& z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    return GetBlockFromMap(terrain, x, y, z) == (u8)Blocks::AIR_BLOCK;
  }
  return false;
}

bool World::isBlockTransparentAtPosition(const float& x, const float& y,
                                         const float& z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    const Blocks blockType =
        static_cast<Blocks>(GetBlockFromMap(terrain, x, y, z));
    return (blockType == Blocks::VOID || isTransparent(blockType) ||
            blockType == Blocks::OAK_LEAVES_BLOCK ||
            blockType == Blocks::BIRCH_LEAVES_BLOCK);
  } else {
    return false;
  }
}

bool World::isTopFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y + 1,
                                      t_blockOffset->z);
}

bool World::isBottomFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y - 1,
                                      t_blockOffset->z);
}

bool World::isFrontFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z - 1);
}

bool World::isBackFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z + 1);
}

bool World::isLeftFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x + 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

bool World::isRightFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x - 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

int World::getBlockVisibleFaces(const Vec4* t_blockOffset) {
  int result = 0x000000;

  // Front
  if (isFrontFaceVisible(t_blockOffset)) result = result | FRONT_VISIBLE;

  // Back
  if (isBackFaceVisible(t_blockOffset)) result = result | BACK_VISIBLE;

  // Right
  if (isRightFaceVisible(t_blockOffset)) result = result | RIGHT_VISIBLE;

  // Left
  if (isLeftFaceVisible(t_blockOffset)) result = result | LEFT_VISIBLE;

  // Top
  if (isTopFaceVisible(t_blockOffset)) result = result | TOP_VISIBLE;

  // Bottom
  if (isBottomFaceVisible(t_blockOffset)) result = result | BOTTOM_VISIBLE;

  // printf("Result for index %i -> 0x%X\n", blockIndex, result);
  return result;
}

int World::getWaterBlockVisibleFaces(const Vec4* t_blockOffset) {
  int result = 0x000000;

  // Front
  if (isAirAtPosition(t_blockOffset->x, t_blockOffset->y, t_blockOffset->z - 1))
    result = result | FRONT_VISIBLE;

  // Back
  if (isAirAtPosition(t_blockOffset->x, t_blockOffset->y, t_blockOffset->z + 1))
    result = result | BACK_VISIBLE;

  // Right
  if (isAirAtPosition(t_blockOffset->x - 1, t_blockOffset->y, t_blockOffset->z))
    result = result | RIGHT_VISIBLE;

  // Left
  if (isAirAtPosition(t_blockOffset->x + 1, t_blockOffset->y, t_blockOffset->z))
    result = result | LEFT_VISIBLE;

  // Top
  if (isAirAtPosition(t_blockOffset->x, t_blockOffset->y + 1, t_blockOffset->z))
    result = result | TOP_VISIBLE;

  // Bottom
  if (isAirAtPosition(t_blockOffset->x, t_blockOffset->y - 1, t_blockOffset->z))
    result = result | BOTTOM_VISIBLE;

  // printf("Result for index %i -> 0x%X\n", blockIndex, result);
  return result;
}

void World::calcRawBlockBBox(MinecraftPipeline* mcPip) {
  const auto& blockData = mcPip->getBlockData();
  rawBlockBbox = new BBox(blockData.vertices, blockData.count);
}

const Vec4 World::defineSpawnArea() {
  Vec4 spawPos = calcSpawOffset();

  level.map.spawnX = spawPos.x;
  level.map.spawnY = spawPos.y;
  level.map.spawnZ = spawPos.z;

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

void World::removeBlock(Block* blockToRemove) {
  Vec4 offsetToRemove;
  GetXYZFromPos(&blockToRemove->offset, &offsetToRemove);

  SetBlockInMapByIndex(terrain, blockToRemove->index, (u8)Blocks::AIR_BLOCK);

  // Generate amount of particles right begore block gets destroyed
  particlesManager.createBlockParticleBatch(blockToRemove, 24);

  // Update sunlight and block light at position
  removeLight(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);
  checkSunLightAt(offsetToRemove.x, offsetToRemove.y, offsetToRemove.z);

  updateSunlight();
  updateBlockLights();

  chunckManager.reloadLightData();

  updateNeighBorsChunksByModdedPosition(offsetToRemove);
  playDestroyBlockSound(blockToRemove->type);

  // Remove up block if it's is vegetation
  const Vec4 upBlockOffset =
      Vec4(offsetToRemove.x, offsetToRemove.y + 1, offsetToRemove.z);

  if (BoundCheckMap(terrain, upBlockOffset.x, upBlockOffset.y,
                    upBlockOffset.z)) {
    const Blocks b = static_cast<Blocks>(GetBlockFromMap(
        terrain, upBlockOffset.x, upBlockOffset.y, upBlockOffset.z));

    if (isVegetation(b)) {
      auto chunk =
          chunckManager.getChunckByPosition(upBlockOffset * DUBLE_BLOCK_SIZE);
      Block* upperBlock = chunk->getBlockByOffset(&upBlockOffset);
      if (upperBlock) removeBlock(upperBlock);
    }
  }
}

void World::putBlock(const Blocks& blockToPlace, Player* t_player) {
  Vec4 targetPos = ray.at(targetBlock->distance);

  Vec4 blockOffset;
  GetXYZFromPos(&targetBlock->offset, &blockOffset);

  // Front
  if (std::round(targetPos.z) ==
      targetBlock->bbox->getFrontFace().axisPosition) {
    blockOffset.z++;
    // Back
  } else if (std::round(targetPos.z) ==
             targetBlock->bbox->getBackFace().axisPosition) {
    blockOffset.z--;
    // Right
  } else if (std::round(targetPos.x) ==
             targetBlock->bbox->getRightFace().axisPosition) {
    blockOffset.x++;
    // Left
  } else if (std::round(targetPos.x) ==
             targetBlock->bbox->getLeftFace().axisPosition) {
    blockOffset.x--;
    // Up
  } else if (std::round(targetPos.y) ==
             targetBlock->bbox->getTopFace().axisPosition) {
    blockOffset.y++;
    // Down
  } else if (std::round(targetPos.y) ==
             targetBlock->bbox->getBottomFace().axisPosition) {
    blockOffset.y--;
  }

  // Is a valid index?
  if (BoundCheckMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z)) {
    Vec4 newBlockPos = blockOffset * DUBLE_BLOCK_SIZE;
    {
      // Prevent to put a block at the player position;
      M4x4 tempModel = M4x4();
      tempModel.identity();
      tempModel.scale(BLOCK_SIZE);
      tempModel.translate(newBlockPos);

      BBox tempBBox = rawBlockBbox->getTransformed(tempModel);
      Vec4 newBlockPosMin;
      Vec4 newBlockPosMax;
      tempBBox.getMinMax(&newBlockPosMin, &newBlockPosMax);

      Vec4 minPlayerCorner;
      Vec4 maxPlayerCorner;
      t_player->getHitBox().getMinMax(&minPlayerCorner, &maxPlayerCorner);

      // Will Collide to player?
      if (newBlockPosMax.x > minPlayerCorner.x &&
          newBlockPosMin.x < maxPlayerCorner.x &&
          newBlockPosMax.z > minPlayerCorner.z &&
          newBlockPosMin.z < maxPlayerCorner.z &&
          newBlockPosMax.y > minPlayerCorner.y &&
          newBlockPosMin.y < maxPlayerCorner.y)
        return;  // Return on collision
    }
    const Blocks blockTypeAtTargetPosition = static_cast<Blocks>(
        GetBlockFromMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z));

    if (blockTypeAtTargetPosition == Blocks::AIR_BLOCK) {
      SetBlockInMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z,
                    static_cast<u8>(blockToPlace));

      removeSunLight(blockOffset.x, blockOffset.y, blockOffset.z);
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
    }

    playPutBlockSound(blockToPlace);
  }

  updateNeighBorsChunksByModdedPosition(blockOffset);
}

void World::stopBreakTargetBlock() {
  _isBreakingBlock = false;
  breaking_time_pessed = 0;
  if (targetBlock) targetBlock->damage = 0;
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

u8 World::isBlockAtChunkBorder(const Vec4* blockOffset,
                               const Vec4* chunkMinOffset,
                               const Vec4* chunkMaxOffset) {
  return blockOffset->y == chunkMinOffset->y ||
         blockOffset->y == chunkMaxOffset->y - 1 ||
         blockOffset->x == chunkMinOffset->x ||
         blockOffset->x == chunkMaxOffset->x - 1 ||
         blockOffset->z == chunkMinOffset->z ||
         blockOffset->z == chunkMaxOffset->z - 1;
}

u8 World::isCrossedBlock(Blocks block_type) {
  return block_type == Blocks::POPPY_FLOWER ||
         block_type == Blocks::DANDELION_FLOWER || block_type == Blocks::GRASS;
}

void World::buildChunk(Chunck* t_chunck) {
  t_chunck->preAllocateMemory();

  for (size_t x = t_chunck->minOffset->x; x < t_chunck->maxOffset->x; x++) {
    for (size_t z = t_chunck->minOffset->z; z < t_chunck->maxOffset->z; z++) {
      for (size_t y = t_chunck->minOffset->y; y < t_chunck->maxOffset->y; y++) {
        u32 blockIndex = getIndexByOffset(x, y, z);
        const Blocks block_type =
            static_cast<Blocks>(terrain->blocks[blockIndex]);

        if (block_type != Blocks::VOID && block_type != Blocks::AIR_BLOCK &&
            block_type != Blocks::TOTAL_OF_BLOCKS) {
          Vec4 tempBlockOffset = Vec4(x, y, z);

          const int visibleFaces =
              block_type == Blocks::WATER_BLOCK
                  ? getWaterBlockVisibleFaces(&tempBlockOffset)
                  : getBlockVisibleFaces(&tempBlockOffset);

          // Have any face visible?
          if (visibleFaces > 0) {
            BlockInfo* blockInfo = blockManager.getBlockInfoByType(block_type);

            if (blockInfo) {
              Block* block = new Block(blockInfo);
              block->index = blockIndex;
              block->offset = GetPosFromXYZ(
                  tempBlockOffset.x, tempBlockOffset.y, tempBlockOffset.z);
              block->chunkId = t_chunck->id;

              if (block->isCrossed) {
                block->visibleFaces = 0x111111;
                block->visibleFacesCount = 2;
              } else {
                block->visibleFaces = visibleFaces;
                block->visibleFacesCount = Utils::countSetBits(visibleFaces);
              }

              block->isAtChunkBorder = isBlockAtChunkBorder(
                  &tempBlockOffset, t_chunck->minOffset, t_chunck->maxOffset);

              block->position.set(tempBlockOffset * DUBLE_BLOCK_SIZE);

              // Calc min and max corners
              {
                M4x4 model;

                model.identity();
                model.scale(BLOCK_SIZE);
                model.translate(block->position);

                BBox tempBBox = rawBlockBbox->getTransformed(model);
                block->bbox = new BBox(tempBBox);
                block->bbox->getMinMax(&block->minCorner, &block->maxCorner);
              }

              t_chunck->addBlock(block);
            }
          }
        }
      }
    }
  }

  t_chunck->loadDrawData();
  t_chunck->freeUnusedMemory();
}

void World::buildChunkAsync(Chunck* t_chunck, const u8& loading_speed) {
  uint16_t safeWhileBreak = 0;
  uint16_t batchCounter = 0;
  uint16_t x = t_chunck->tempLoadingOffset->x;
  uint16_t y = t_chunck->tempLoadingOffset->y;
  uint16_t z = t_chunck->tempLoadingOffset->z;
  auto limit = LOAD_CHUNK_BATCH;

  if (!t_chunck->isPreAllocated()) t_chunck->preAllocateMemory();

  while (batchCounter < limit) {
    if (x >= t_chunck->maxOffset->x) break;
    safeWhileBreak++;

    u32 blockIndex = getIndexByOffset(x, y, z);
    const Blocks block_type = static_cast<Blocks>(terrain->blocks[blockIndex]);

    if (block_type != Blocks::VOID && block_type != Blocks::AIR_BLOCK &&
        block_type != Blocks::TOTAL_OF_BLOCKS) {
      Vec4 tempBlockOffset = Vec4(x, y, z);

      const int visibleFaces = block_type == Blocks::WATER_BLOCK
                                   ? getWaterBlockVisibleFaces(&tempBlockOffset)
                                   : getBlockVisibleFaces(&tempBlockOffset);

      // Is any face vísible?
      if (visibleFaces > 0) {
        BlockInfo* blockInfo = blockManager.getBlockInfoByType(block_type);

        if (blockInfo) {
          Block* block = new Block(blockInfo);
          block->index = blockIndex;
          block->offset = GetPosFromXYZ(tempBlockOffset.x, tempBlockOffset.y,
                                        tempBlockOffset.z);
          block->chunkId = t_chunck->id;

          if (block->isCrossed) {
            block->visibleFaces = 0x111111;
            block->visibleFacesCount = 2;
          } else {
            block->visibleFaces = visibleFaces;
            block->visibleFacesCount = Utils::countSetBits(visibleFaces);
          }

          block->isAtChunkBorder = isBlockAtChunkBorder(
              &tempBlockOffset, t_chunck->minOffset, t_chunck->maxOffset);

          block->position.set(tempBlockOffset * DUBLE_BLOCK_SIZE);

          // Calc min and max corners
          {
            M4x4 model;

            model.identity();
            model.scale(BLOCK_SIZE);
            model.translate(block->position);

            BBox tempBBox = rawBlockBbox->getTransformed(model);
            block->bbox = new BBox(tempBBox);
            block->bbox->getMinMax(&block->minCorner, &block->maxCorner);
          }

          t_chunck->addBlock(block);
        }
        batchCounter++;
      }
    }

    y++;
    if (y >= t_chunck->maxOffset->y) {
      y = t_chunck->minOffset->y;
      z++;
    }
    if (z >= t_chunck->maxOffset->z) {
      z = t_chunck->minOffset->z;
      x++;
    }

    if (safeWhileBreak > CHUNCK_LENGTH) {
      TYRA_WARN("Safely breaking while loop");
      break;
    }
  }

  if (batchCounter >= limit) {
    t_chunck->tempLoadingOffset->set(x, y, z);
    return;
  }

  t_chunck->state = ChunkState::PreLoaded;
  t_chunck->freeUnusedMemory();
}

void World::updateTargetBlock(Camera* t_camera, Player* t_player,
                              const std::vector<Chunck*>& chuncks) {
  const Vec4 baseOrigin =
      *t_player->getPosition() + Vec4(0.0f, t_camera->getCamY(), 0.0f);
  u8 hitedABlock = 0;
  float tempTargetDistance = -1.0f;
  float tempPlayerDistance = -1.0f;
  Block* tempTargetBlock = nullptr;
  uint32_t _lastTargetBlockId = 0;

  if (targetBlock) {
    _lastTargetBlockId = targetBlock->index;
    // (targetBlock->offset.y * terrain->length * terrain->width) +
    // (targetBlock->offset.z * terrain->width) + ;
  }

  // Reset the current target block;
  targetBlock = nullptr;

  // Prepate the raycast
  ray.origin.set(baseOrigin);
  ray.direction.set(t_camera->unitCirclePosition.getNormalized());

  // Reverse ray for third person camera position
  Ray revRay;
  revRay.direction = (-t_camera->unitCirclePosition).getNormalized();
  revRay.origin = baseOrigin;

  t_camera->hitDistance = t_camera->getDistanceFromPlayer();

  for (u16 h = 0; h < chuncks.size(); h++) {
    for (u16 i = 0; i < chuncks[h]->blocks.size(); i++) {
      Block* block = chuncks[h]->blocks[i];

      u8 isBreakable = block->type != Blocks::WATER_BLOCK;
      float distanceFromCurrentBlockToPlayer =
          baseOrigin.distanceTo(block->position);

      // Reset block state
      block->isTarget = false;
      block->distance = -1.0f;

      if (isBreakable && distanceFromCurrentBlockToPlayer <= MAX_RANGE_PICKER) {
        float intersectionPoint;
        if (ray.intersectBox(block->minCorner, block->maxCorner,
                             &intersectionPoint)) {
          hitedABlock = 1;
          if (tempTargetDistance == -1.0f ||
              (distanceFromCurrentBlockToPlayer < tempPlayerDistance)) {
            tempTargetBlock = block;
            tempTargetDistance = intersectionPoint;
            tempPlayerDistance = distanceFromCurrentBlockToPlayer;
          }
        }
      }

      if (block->isCollidable) {
        float intersectionPoint;
        if (revRay.intersectBox(block->minCorner, block->maxCorner,
                                &intersectionPoint)) {
          if (intersectionPoint < t_camera->hitDistance) {
            t_camera->hitDistance = intersectionPoint * 0.95F;
          }
        }
      }
    }
  }

  if (hitedABlock) {
    targetBlock = tempTargetBlock;
    targetBlock->isTarget = true;
    targetBlock->distance = tempTargetDistance;
    targetBlock->hitPosition.set(ray.at(tempTargetDistance));

    const uint32_t _hitedBlockId = targetBlock->index;
    // (targetBlock->offset.y * terrain->length * terrain->width) +
    // (targetBlock->offset.z * terrain->width) + targetBlock->offset.x;
    if (_hitedBlockId != _lastTargetBlockId) breaking_time_pessed = 0;
  }
}

void World::setDrawDistace(const u8& drawDistanceInChunks) {
  if (drawDistanceInChunks >= MIN_DRAW_DISTANCE &&
      drawDistanceInChunks <= MAX_DRAW_DISTANCE) {
    worldOptions.drawDistance = drawDistanceInChunks;
    Chunck* currentChunck =
        chunckManager.getChunckByPosition(lastPlayerPosition);
    if (currentChunck)
      scheduleChunksNeighbors(currentChunck, lastPlayerPosition, true);
  }
}

void World::initWorldLightModel() {
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.sunLightIntensity =
      dayNightCycleManager.getSunLightIntensity();
}

// From CrossCraft
struct LightNode {
  uint16_t x, y, z;
  LightNode(uint16_t lx, uint16_t ly, uint16_t lz, uint16_t l)
      : x(lx), y(ly), z(lz), val(l) {}
  uint16_t val;
};

std::queue<LightNode> lightBfsQueue;
std::queue<LightNode> lightRemovalBfsQueue;

// std::stack<LightNode> sunlightBfsQueue;
std::queue<LightNode> sunlightBfsQueue;
std::queue<LightNode> sunlightRemovalBfsQueue;

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
    LightNode lightNode = sunlightRemovalBfsQueue.front();
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
    LightNode lightNode = lightRemovalBfsQueue.front();
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

  uint32_t blockCount = OVERWORLD_SIZE;
  LevelMap map = {.width = OVERWORLD_H_DISTANCE,
                  .length = OVERWORLD_H_DISTANCE,
                  .height = OVERWORLD_V_DISTANCE,

                  .spawnX = 128,
                  .spawnY = 59,
                  .spawnZ = 128,

                  .blocks = new u8[blockCount],
                  .lightData = new u8[blockCount],
                  .data = new u8[blockCount]};

  level.map = map;

  // For some reason I need to clear the array garbage
  // I was initialized with new key word, very weird!
  for (size_t i = 0; i < blockCount; i++) {
    level.map.blocks[i] = 0;
    level.map.lightData[i] = 0;
    level.map.data[i] = 0;
  }

  TYRA_LOG("Generated base level template");
}

void CrossCraft_World_Deinit() {
  TYRA_LOG("Destroying the world");
  if (level.map.blocks) delete[] level.map.blocks;
  if (level.map.data) delete[] level.map.data;
  if (level.map.lightData) delete[] level.map.lightData;
  TYRA_LOG("World freed");
}

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void CrossCraft_World_GenerateMap(WorldType worldType) {
  switch (worldType) {
    case WORLD_TYPE_ORIGINAL:
      CrossCraft_WorldGenerator_Generate_Original(&level.map);
      break;
    case WORLD_TYPE_FLAT:
      CrossCraft_WorldGenerator_Generate_Flat(&level.map);
      break;
    case WORLD_TYPE_ISLAND:
      CrossCraft_WorldGenerator_Generate_Island(&level.map);
      break;
    case WORLD_TYPE_WOODS:
      CrossCraft_WorldGenerator_Generate_Woods(&level.map);
      break;
    case WORLD_TYPE_FLOATING:
      CrossCraft_WorldGenerator_Generate_Floating(&level.map);
      break;
  }
}

LevelMap* CrossCraft_World_GetMapPtr() { return &level.map; }