
#include "entities/World.hpp"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"
#include <tyra>

// From CrossCraft
#include <stdio.h>
#include "entities/World.hpp"
#include <queue>

using Tyra::Color;
using Tyra::M4x4;

World::World(const NewGameOptions& options) {
  seed = options.seed;

  printf("\n\n|-----------SEED---------|");
  printf("\n|           %ld         |\n", seed);
  printf("|------------------------|\n\n");

  worldOptions = options;
  CrossCraft_World_Init(seed);
}

World::~World() {
  delete rawBlockBbox;
  CrossCraft_World_Deinit();
}

void World::init(Renderer* renderer, ItemRepository* itemRepository,
                 SoundManager* t_soundManager) {
  t_renderer = renderer;
  mcPip.setRenderer(&t_renderer->core);
  stapip.setRenderer(&t_renderer->core);
  blockManager.init(t_renderer, &mcPip, worldOptions.texturePack);
  chunckManager.init();
  cloudsManager.init(t_renderer);
  calcRawBlockBBox(&mcPip);

  terrain = CrossCraft_World_GetMapPtr();
  CrossCraft_World_Create_Map();
  CrossCraft_World_GenerateMap(worldOptions.type);

  // Define global and local spawn area
  worldSpawnArea.set(defineSpawnArea());
  spawnArea.set(worldSpawnArea);
  lastPlayerPosition.set(worldSpawnArea);
  buildInitialPosition();
  setIntialTime();
};

void World::update(Player* t_player, const Vec4& camLookPos,
                   const Vec4& camPosition) {
  framesCounter++;

  cloudsManager.update();
  dayNightCycleManager.update();
  updateLightModel();

  chunckManager.update(t_renderer->core.renderer3D.frustumPlanes.getAll(),
                       *t_player->getPosition(), &worldLightModel);
  updateChunkByPlayerPosition(t_player);
  updateTargetBlock(camLookPos, camPosition, chunckManager.getVisibleChunks());

  framesCounter %= 60;
};

void World::render() {
  t_renderer->core.setClearScreenColor(dayNightCycleManager.getSkyColor());

  chunckManager.renderer(t_renderer, &stapip, &blockManager);
  cloudsManager.render();

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
  }
};

void World::resetWorldData() { chunckManager.clearAllChunks(); }

const std::vector<Block*> World::getLoadedBlocks() {
  loadedBlocks.clear();
  loadedBlocks.shrink_to_fit();
  const auto& visibleChuncks = chunckManager.getVisibleChunks();
  for (u16 i = 0; i < visibleChuncks.size(); i++) {
    for (u16 j = 0; j < visibleChuncks[i]->blocks.size(); j++)
      loadedBlocks.push_back(visibleChuncks[i]->blocks[j]);
  }
  return loadedBlocks;
}

void World::updateChunkByPlayerPosition(Player* t_player) {
  Vec4 currentPlayerPos = *t_player->getPosition();
  if (lastPlayerPosition.distanceTo(currentPlayerPos) > CHUNCK_SIZE) {
    lastPlayerPosition.set(currentPlayerPos);
    Chunck* currentChunck = chunckManager.getChunckByPosition(currentPlayerPos);

    if (currentChunck && t_player->currentChunckId != currentChunck->id) {
      t_player->currentChunckId = currentChunck->id;
      scheduleChunksNeighbors(currentChunck, currentPlayerPos);
    }
  }

  if (framesCounter % 2 == 0) {
    unloadScheduledChunks();
    loadScheduledChunks();
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
    float distance =
        floor(t_chunck->center->distanceTo(*chuncks[i]->center) / CHUNCK_SIZE) +
        1;

    if (distance > worldOptions.drawDistance) {
      if (force_loading)
        chuncks[i]->clear();
      else if (chuncks[i]->state != ChunkState::Clean)
        addChunkToUnloadAsync(chuncks[i]);
    } else {
      if (force_loading) {
        chuncks[i]->clear();
        buildChunkAsync(chuncks[i], worldOptions.drawDistance);
      } else if (chuncks[i]->state != ChunkState::Loaded) {
        addChunkToLoadAsync(chuncks[i]);
      }
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
    if (tempChuncksToLoad[0]->state != ChunkState::Loaded)
      return buildChunkAsync(tempChuncksToLoad[0], worldOptions.drawDistance);
    tempChuncksToLoad.erase(tempChuncksToLoad.begin());
    chunckManager.sortChunkByPlayerPosition(&lastPlayerPosition);
  };
}

void World::unloadScheduledChunks() {
  const size_t size = tempChuncksToUnLoad.size();
  const u8 limit = 2;
  u8 counter = 0;

  for (size_t i = 0; i < size; i++) {
    if (tempChuncksToUnLoad[i]->state != ChunkState::Clean) {
      tempChuncksToUnLoad[i]->clear();
      counter++;
      if (counter >= limit) return;
    }
  }

  tempChuncksToUnLoad.clear();
  tempChuncksToUnLoad.shrink_to_fit();
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
    overlayData.shrink_to_fit();
  }

  M4x4 scale = M4x4();
  M4x4 translation = M4x4();

  scale.identity();
  scale.scale(BLOCK_SIZE + 0.015f);

  translation.identity();
  translation.translate(*targetBlock->getPosition());

  overlay->model = new M4x4(translation * scale);
  overlay->color = new Color(128.0f, 128.0f, 128.0f, 70.0f);

  overlayData.push_back(overlay);
  t_renderer->renderer3D.usePipeline(&mcPip);
  mcPip.render(overlayData, blockManager.getBlocksTexture(), false);
}

void World::renderTargetBlockHitbox(Block* targetBlock) {
  t_renderer->renderer3D.utility.drawBox(*targetBlock->getPosition(),
                                         BLOCK_SIZE, Color(0, 0, 0));
}

void World::addChunkToLoadAsync(Chunck* t_chunck) {
  // Avoid being suplicated;
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  t_chunck->state = ChunkState::Loading;
  tempChuncksToLoad.push_back(t_chunck);
}

void World::addChunkToUnloadAsync(Chunck* t_chunck) {
  // Avoid being duplicated;
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id)
      tempChuncksToLoad.erase(tempChuncksToLoad.begin() + i);

  tempChuncksToUnLoad.push_back(t_chunck);
}

void World::updateLightModel() {
  worldLightModel.lightsPositions = lightsPositions.data();
  worldLightModel.lightIntensity = dayNightCycleManager.getLightIntensity();
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.ambientLightIntensity =
      dayNightCycleManager.getAmbientLightIntesity();
}

unsigned int World::getIndexByOffset(int x, int y, int z) {
  return (y * terrain->length * terrain->width) + (z * terrain->width) + x;
}

bool World::isAirAtPosition(const float& x, const float& y, const float& z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    const u8 blockType = GetBlockFromMap(terrain, x, y, z);
    return blockType == (u8)Blocks::AIR_BLOCK;
  }
  return false;
}

bool World::isBlockTransparentAtPosition(const float& x, const float& y,
                                         const float& z) {
  if (BoundCheckMap(terrain, x, y, z)) {
    const u8 blockType = GetBlockFromMap(terrain, x, y, z);
    return blockType <= (u8)Blocks::AIR_BLOCK ||
           blockManager.isBlockTransparent(static_cast<Blocks>(blockType));
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
  return isBlockTransparentAtPosition(t_blockOffset->x - 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

bool World::isBackFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x + 1, t_blockOffset->y,
                                      t_blockOffset->z);
}

bool World::isLeftFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z - 1);
}

bool World::isRightFaceVisible(const Vec4* t_blockOffset) {
  return isBlockTransparentAtPosition(t_blockOffset->x, t_blockOffset->y,
                                      t_blockOffset->z + 1);
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
  if (isAirAtPosition(t_blockOffset->x - 1, t_blockOffset->y, t_blockOffset->z))
    result = result | FRONT_VISIBLE;

  // Back
  if (isAirAtPosition(t_blockOffset->x + 1, t_blockOffset->y, t_blockOffset->z))
    result = result | BACK_VISIBLE;

  // Right
  if (isAirAtPosition(t_blockOffset->x, t_blockOffset->y, t_blockOffset->z + 1))
    result = result | RIGHT_VISIBLE;

  // Left
  if (isAirAtPosition(t_blockOffset->x, t_blockOffset->y, t_blockOffset->z - 1))
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
  SetBlockInMap(terrain, blockToRemove->offset.x, blockToRemove->offset.y,
                blockToRemove->offset.z, (u8)Blocks::AIR_BLOCK);
  updateNeighBorsChunksByModdedPosition(blockToRemove->offset);
  // playDestroyBlockSound(blockToRemove->type);
}

void World::putBlock(const Blocks& blockToPlace, Player* t_player) {
  Vec4 targetPos = ray.at(targetBlock->distance);
  Vec4 blockOffset = Vec4(targetBlock->offset);

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
    const uint8_t blockType =
        GetBlockFromMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z);
    if (blockType == (u8)Blocks::AIR_BLOCK) {
      SetBlockInMap(terrain, blockOffset.x, blockOffset.y, blockOffset.z,
                    (u8)blockToPlace);
    }

    // playPutBlockSound(blockToPlace);
  }

  updateNeighBorsChunksByModdedPosition(blockOffset);
}

void World::stopBreakTargetBlock() {
  _isBreakingBlock = false;
  if (targetBlock) targetBlock->damage = 0;
}

void World::breakTargetBlock(const float& deltaTime) {
  if (targetBlock == nullptr) return;

  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;

    if (breaking_time_pessed >= blockManager.getBlockBreakingTime()) {
      // Remove block;
      removeBlock(targetBlock);

      // Target block has changed, reseting the pressed time;
      breaking_time_pessed = 0;
    } else {
      // Update damage overlay
      targetBlock->damage =
          breaking_time_pessed / blockManager.getBlockBreakingTime() * 100;
      if (lastTimePlayedBreakingSfx > 0.3F) {
        // playBreakingBlockSound(targetBlock->type);
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
    if (blockSfxModel != nullptr) {
      const int ch = t_soundManager->getAvailableChannel();
      t_soundManager->playSfx(blockSfxModel->category, blockSfxModel->sound,
                              ch);
    }
    Tyra::Threading::switchThread();
  }
}

void World::playDestroyBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getDigSoundByBlockType(blockType);

    if (blockSfxModel != nullptr) {
      const int ch = t_soundManager->getAvailableChannel();
      t_soundManager->playSfx(blockSfxModel->category, blockSfxModel->sound,
                              ch);
    }
    Tyra::Threading::switchThread();
  }
}

void World::playBreakingBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        blockManager.getDigSoundByBlockType(blockType);

    if (blockSfxModel != nullptr) {
      const int ch = t_soundManager->getAvailableChannel();
      t_soundManager->playSfx(blockSfxModel->category, blockSfxModel->sound,
                              ch);
    }
    Tyra::Threading::switchThread();
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

void World::buildChunk(Chunck* t_chunck) {
  for (size_t x = t_chunck->minOffset->x; x < t_chunck->maxOffset->x; x++) {
    for (size_t z = t_chunck->minOffset->z; z < t_chunck->maxOffset->z; z++) {
      for (size_t y = t_chunck->minOffset->y; y < t_chunck->maxOffset->y; y++) {
        unsigned int blockIndex = getIndexByOffset(x, y, z);
        u8 block_type = GetBlockFromMap(terrain, x, y, z);

        if (block_type <= (u8)Blocks::AIR_BLOCK ||
            !BoundCheckMap(terrain, x, y, z))
          continue;

        Vec4 tempBlockOffset = Vec4(x, y, z);
        Vec4 blockPosition = (tempBlockOffset * DUBLE_BLOCK_SIZE);

        const int visibleFaces =
            block_type == (u8)Blocks::WATER_BLOCK
                ? getWaterBlockVisibleFaces(&tempBlockOffset)
                : getBlockVisibleFaces(&tempBlockOffset);

        const bool isVisible = visibleFaces > 0;

        // Are block's coordinates in world range?
        if (isVisible) {
          BlockInfo* blockInfo =
              blockManager.getBlockInfoByType(static_cast<Blocks>(block_type));
          if (blockInfo) {
            Block* block = new Block(blockInfo);
            block->index = blockIndex;
            block->offset.set(tempBlockOffset);
            block->chunkId = t_chunck->id;
            block->visibleFaces = visibleFaces;
            block->isAtChunkBorder = isBlockAtChunkBorder(
                &tempBlockOffset, t_chunck->minOffset, t_chunck->maxOffset);

            block->setPosition(blockPosition);
            block->scale.scale(BLOCK_SIZE);
            block->updateModelMatrix();

            // Calc min and max corners
            {
              BBox tempBBox = rawBlockBbox->getTransformed(block->model);
              block->bbox = new BBox(tempBBox);
              block->bbox->getMinMax(&block->minCorner, &block->maxCorner);
            }

            t_chunck->addBlock(block);
          }
        }
      }
    }
  }

  t_chunck->state = ChunkState::Loaded;
  t_chunck->loadDrawData();
}

void World::buildChunkAsync(Chunck* t_chunck, const u8& loading_speed) {
  uint16_t safeWhileBreak = 0;
  uint16_t batchCounter = 0;
  uint16_t x = t_chunck->tempLoadingOffset->x;
  uint16_t y = t_chunck->tempLoadingOffset->y;
  uint16_t z = t_chunck->tempLoadingOffset->z;

  while (batchCounter < LOAD_CHUNK_BATCH * loading_speed) {
    if (x >= t_chunck->maxOffset->x) break;
    safeWhileBreak++;

    unsigned int blockIndex = getIndexByOffset(x, y, z);
    u8 block_type = GetBlockFromMap(terrain, x, y, z);
    if (block_type > (u8)Blocks::AIR_BLOCK &&
        block_type < (u8)Blocks::TOTAL_OF_BLOCKS) {
      Vec4 tempBlockOffset = Vec4(x, y, z);
      Vec4 blockPosition = (tempBlockOffset * DUBLE_BLOCK_SIZE);

      const int visibleFaces = block_type == (u8)Blocks::WATER_BLOCK
                                   ? getWaterBlockVisibleFaces(&tempBlockOffset)
                                   : getBlockVisibleFaces(&tempBlockOffset);

      const bool isVisible = visibleFaces > 0;

      // Are block's coordinates in world range?
      if (isVisible && BoundCheckMap(terrain, x, y, z)) {
        BlockInfo* blockInfo =
            blockManager.getBlockInfoByType(static_cast<Blocks>(block_type));

        if (blockInfo) {
          Block* block = new Block(blockInfo);
          block->index = blockIndex;
          block->offset.set(tempBlockOffset);
          block->chunkId = t_chunck->id;
          block->visibleFaces = visibleFaces;
          block->isAtChunkBorder = isBlockAtChunkBorder(
              &tempBlockOffset, t_chunck->minOffset, t_chunck->maxOffset);

          block->setPosition(blockPosition);
          block->scale.scale(BLOCK_SIZE);
          block->updateModelMatrix();

          // Calc min and max corners
          {
            BBox tempBBox = rawBlockBbox->getTransformed(block->model);
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

    if (safeWhileBreak > CHUNCK_LENGTH) break;
  }

  if (batchCounter >= LOAD_CHUNK_BATCH * loading_speed) {
    t_chunck->tempLoadingOffset->set(x, y, z);
    return;
  }

  t_chunck->loadDrawData();
  t_chunck->state = ChunkState::Loaded;
}

void World::updateTargetBlock(const Vec4& camLookPos, const Vec4& camPosition,
                              const std::vector<Chunck*>& chuncks) {
  u8 hitedABlock = 0;
  float tempTargetDistance = -1.0f;
  float tempPlayerDistance = -1.0f;
  Block* tempTargetBlock = nullptr;

  // Reset the current target block;
  targetBlock = nullptr;

  // Prepate the raycast
  Vec4 rayDir = camLookPos - camPosition;
  rayDir.normalize();
  ray.origin.set(camPosition);
  ray.direction.set(rayDir);

  for (u16 h = 0; h < chuncks.size(); h++) {
    for (u16 i = 0; i < chuncks[h]->blocks.size(); i++) {
      float distanceFromCurrentBlockToPlayer =
          camPosition.distanceTo(*chuncks[h]->blocks[i]->getPosition());

      if (distanceFromCurrentBlockToPlayer <= MAX_RANGE_PICKER) {
        // Reset block state
        chuncks[h]->blocks[i]->isTarget = 0;
        chuncks[h]->blocks[i]->distance = -1.0f;

        float intersectionPoint;
        if (ray.intersectBox(chuncks[h]->blocks[i]->minCorner,
                             chuncks[h]->blocks[i]->maxCorner,
                             &intersectionPoint)) {
          hitedABlock = 1;
          if (tempTargetDistance == -1.0f ||
              (distanceFromCurrentBlockToPlayer < tempPlayerDistance)) {
            tempTargetBlock = chuncks[h]->blocks[i];
            tempTargetDistance = intersectionPoint;
            tempPlayerDistance = distanceFromCurrentBlockToPlayer;
          }
        }
      }
    }
  }

  if (hitedABlock) {
    targetBlock = tempTargetBlock;
    targetBlock->isTarget = 1;
    targetBlock->distance = tempTargetDistance;
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

// From CrossCraft
struct LightNode {
  uint16_t x, y, z;
  LightNode(uint16_t lx, uint16_t ly, uint16_t lz, uint16_t l)
      : x(lx), y(ly), z(lz), val(l) {}
  uint16_t val;
};

std::queue<LightNode> lightBfsQueue;
std::queue<LightNode> lightRemovalBfsQueue;

std::queue<LightNode> sunlightBfsQueue;
std::queue<LightNode> sunlightRemovalBfsQueue;

auto encodeID(uint16_t x, uint16_t z) -> uint32_t {
  uint16_t nx = x / 16;
  uint16_t ny = z / 16;
  uint32_t id = nx << 16 | (ny & 0xFFFF);
  return id;
}

void checkAddID(uint32_t* updateIDs, uint32_t id) {
  // Check that the ID is not already existing
  for (int i = 0; i < 10; i++) {
    if (id == updateIDs[i]) return;
  }

  // Find a slot to insert.
  for (int i = 0; i < 10; i++) {
    if (updateIDs[i] == 0xFFFFFFFF) {
      updateIDs[i] = id;
      return;
    }
  }
}

void updateID(uint16_t x, uint16_t z, uint32_t* updateIDs) {
  checkAddID(updateIDs, encodeID(x, z));
}

void propagate(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
               uint32_t* updateIDs) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  updateID(x, z, updateIDs);

  auto blk = GetBlockFromMap(map, x, y, z);

  if ((blk == 0 || blk == 20 || blk == 18 || (blk >= 8 && blk <= 11) ||
       (blk >= 37 && blk <= 40)) &&
      GetLightFromMap(map, x, y, z) + 2 <= lightLevel) {
    if (blk == 18 || (blk >= 8 && blk <= 11) || (blk >= 37 && blk <= 40)) {
      lightLevel -= 2;
    }
    SetLightInMap(map, x, y, z, lightLevel - 1);
    lightBfsQueue.emplace(x, y, z, 0);
  }
}

void propagate(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  if (GetBlockFromMap(map, x, y, z) == 0 &&
      GetLightFromMap(map, x, y, z) + 2 <= lightLevel) {
    SetLightInMap(map, x, y, z, lightLevel - 1);
    sunlightBfsQueue.emplace(x, y, z, 0);
  }
}

void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
                     uint32_t* updateIDs) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  auto neighborLevel = GetLightFromMap(map, x, y, z);

  updateID(x, z, updateIDs);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    SetLightInMap(map, x, y, z, 0);
    lightRemovalBfsQueue.emplace(x, y, z, neighborLevel);
  } else if (neighborLevel >= lightLevel) {
    lightBfsQueue.emplace(x, y, z, 0);
  }
}

void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  auto neighborLevel = GetLightFromMap(map, x, y, z);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    SetLightInMap(map, x, y, z, 0);
    sunlightRemovalBfsQueue.emplace(x, y, z, neighborLevel);
  } else if (neighborLevel >= lightLevel) {
    sunlightBfsQueue.emplace(x, y, z, 0);
  }
}

void updateRemove(uint32_t* updateIDs) {
  while (!lightRemovalBfsQueue.empty()) {
    auto node = lightRemovalBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel = node.val;
    lightRemovalBfsQueue.pop();

    propagateRemove(nx + 1, ny, nz, lightLevel, updateIDs);
    propagateRemove(nx - 1, ny, nz, lightLevel, updateIDs);
    propagateRemove(nx, ny + 1, nz, lightLevel, updateIDs);
    propagateRemove(nx, ny - 1, nz, lightLevel, updateIDs);
    propagateRemove(nx, ny, nz + 1, lightLevel, updateIDs);
    propagateRemove(nx, ny, nz - 1, lightLevel, updateIDs);
  }
}

void updateRemove() {
  while (!lightRemovalBfsQueue.empty()) {
    auto node = lightRemovalBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel = node.val;
    lightRemovalBfsQueue.pop();

    propagateRemove(nx + 1, ny, nz, lightLevel);
    propagateRemove(nx - 1, ny, nz, lightLevel);
    propagateRemove(nx, ny + 1, nz, lightLevel);
    propagateRemove(nx, ny - 1, nz, lightLevel);
    propagateRemove(nx, ny, nz + 1, lightLevel);
    propagateRemove(nx, ny, nz - 1, lightLevel);
  }
}

void updateSpread(uint32_t* updateIDs) {
  while (!lightBfsQueue.empty()) {
    auto node = lightBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel =
        GetLightFromMap(CrossCraft_World_GetMapPtr(), nx, ny, nz);
    lightBfsQueue.pop();

    propagate(nx + 1, ny, nz, lightLevel, updateIDs);
    propagate(nx - 1, ny, nz, lightLevel, updateIDs);
    propagate(nx, ny + 1, nz, lightLevel, updateIDs);
    propagate(nx, ny - 1, nz, lightLevel, updateIDs);
    propagate(nx, ny, nz + 1, lightLevel, updateIDs);
    propagate(nx, ny, nz - 1, lightLevel, updateIDs);
  }
}

void updateSpread() {
  while (!lightBfsQueue.empty()) {
    auto node = lightBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel =
        GetLightFromMap(CrossCraft_World_GetMapPtr(), nx, ny, nz);
    lightBfsQueue.pop();

    propagate(nx + 1, ny, nz, lightLevel);
    propagate(nx - 1, ny, nz, lightLevel);
    propagate(nx, ny + 1, nz, lightLevel);
    propagate(nx, ny - 1, nz, lightLevel);
    propagate(nx, ny, nz + 1, lightLevel);
    propagate(nx, ny, nz - 1, lightLevel);
  }
}

void updateSunlight() {
  while (!sunlightBfsQueue.empty()) {
    auto node = sunlightBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    int8_t lightLevel = node.val - 1;
    sunlightBfsQueue.pop();
    if (lightLevel <= 0) continue;

    propagate(nx + 1, ny, nz, lightLevel);
    propagate(nx - 1, ny, nz, lightLevel);
    propagate(nx, ny + 1, nz, lightLevel);
    propagate(nx, ny - 1, nz, lightLevel);
    propagate(nx, ny, nz + 1, lightLevel);
    propagate(nx, ny, nz - 1, lightLevel);
  }
}

void updateSunlightRemove() {
  while (!sunlightRemovalBfsQueue.empty()) {
    auto node = sunlightRemovalBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    int8_t lightLevel = node.val;
    sunlightRemovalBfsQueue.pop();
    if (lightLevel <= 0) continue;

    propagateRemove(nx + 1, ny, nz, lightLevel);
    propagateRemove(nx - 1, ny, nz, lightLevel);
    propagateRemove(nx, ny + 1, nz, lightLevel);
    propagateRemove(nx, ny - 1, nz, lightLevel);
    propagateRemove(nx, ny, nz + 1, lightLevel);
    propagateRemove(nx, ny, nz - 1, lightLevel);
  }
}

void CrossCraft_World_AddLight(uint16_t x, uint16_t y, uint16_t z,
                               uint16_t light, uint32_t* updateIDs) {
  SetLightInMap(CrossCraft_World_GetMapPtr(), x, y, z, light);
  updateID(x, z, updateIDs);
  lightBfsQueue.emplace(x, y, z, light);

  updateSpread(updateIDs);
}

void CrossCraft_World_RemoveLight(uint16_t x, uint16_t y, uint16_t z,
                                  uint16_t light, uint32_t* updateIDs) {
  auto map = CrossCraft_World_GetMapPtr();

  auto val = GetLightFromMap(map, x, y, z);
  lightRemovalBfsQueue.emplace(x, y, z, val);

  SetLightInMap(map, x, y, z, light);
  updateID(x, z, updateIDs);

  updateRemove(updateIDs);
  updateSpread(updateIDs);
}

void singleCheck(uint16_t x, uint16_t y, uint16_t z) {
  auto map = CrossCraft_World_GetMapPtr();

  if (y == 0) return;

  auto lv = 15;
  for (int y2 = map->height - 1; y2 >= 0; y2--) {
    auto blk = GetBlockFromMap(map, x, y2, z);

    if (blk == 18 || (blk >= 37 && blk <= 40) || (blk >= 8 && blk <= 11)) {
      lv -= 2;
    } else if (blk != 0 && blk != 20) {
      lv = 0;
    }

    auto lv2 = GetLightFromMap(map, x, y2, z);

    if (lv2 < lv) {
      SetLightInMap(map, x, y2, z, lv);
      sunlightRemovalBfsQueue.emplace(x, y2, z, 0);
    } else {
      SetLightInMap(map, x, y2, z, lv);
      sunlightBfsQueue.emplace(x, y2, z, lv);
    }
  }
}

bool CrossCraft_World_CheckSunLight(uint16_t x, uint16_t y, uint16_t z) {
  singleCheck(x, y, z);
  singleCheck(x + 1, y, z);
  singleCheck(x - 1, y, z);
  singleCheck(x, y, z + 1);
  singleCheck(x, y, z - 1);

  updateSunlightRemove();
  updateSunlight();

  return true;
}

void CrossCraft_World_PropagateSunLight(uint32_t tick) {
  auto map = CrossCraft_World_GetMapPtr();

  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      auto lv = 4;
      if (tick >= 0 && tick <= 12000) {
        lv = 15;
      }

      for (int y = map->height - 1; y >= 0; y--) {
        auto blk = GetBlockFromMap(map, x, y, z);

        if (blk == 18 || (blk >= 37 && blk <= 40) || (blk >= 8 && blk <= 11)) {
          lv -= 2;
        } else if (blk != 0 && blk != 20) {
          break;
        }

        if (lv < 0) break;

        SetLightInMap(map, x, y, z, lv);
        sunlightBfsQueue.emplace(x, y, z, lv);
      }
    }
  }

  updateSunlight();
}

void CrossCraft_World_Init(const uint32_t& seed) {
  TYRA_LOG("Generating base level template");
  srand(seed);

  CrossCraft_WorldGenerator_Init(rand());

  LevelMap map = {.width = 256,
                  .length = 256,
                  .height = 64,

                  .spawnX = 128,
                  .spawnY = 59,
                  .spawnZ = 128,

                  .blocks = NULL,
                  .data = NULL};
  level.map = map;

  TYRA_LOG("Generated base level template");
}

void CrossCraft_World_Deinit() {
  TYRA_LOG("Destroying the world");
  if (level.map.blocks) delete[] level.map.blocks;
  // if (level.map.data) delete[] level.map.data;
  TYRA_LOG("World freed");
}

void CrossCraft_World_Create_Map() {
  level.map.length = OVERWORLD_H_DISTANCE;
  level.map.width = OVERWORLD_H_DISTANCE;
  level.map.height = OVERWORLD_V_DISTANCE;

  uint32_t blockCount = level.map.length * level.map.height * level.map.width;
  level.map.blocks = new uint8_t[blockCount];
  // level.map.data = new uint8_t[blockCount];
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

/**
 * @brief Spawn the player into the world
 */
void CrossCraft_World_Spawn() {}

LevelMap* CrossCraft_World_GetMapPtr() { return &level.map; }