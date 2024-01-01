#include "managers/chunck_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math/plane.hpp"
#include <algorithm>

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Vec4;

ChunckManager::ChunckManager() {}

ChunckManager::~ChunckManager() {
  for (u16 i = 0; i < chuncks.size(); i++) {
    delete chuncks[i];
    chuncks[i] = NULL;
  }
  chuncks.clear();
  chuncks.shrink_to_fit();

  visibleChunks.clear();
  visibleChunks.shrink_to_fit();
}

void ChunckManager::init(WorldLightModel* t_worldLightModel,
                         LevelMap* t_terrain) {
  worldLightModel = t_worldLightModel;
  terrain = t_terrain;
  this->generateChunks();
}

void ChunckManager::clearAllChunks() {
  for (u16 i = 0; i < chuncks.size(); i++) chuncks[i]->clear();
}

void ChunckManager::update(const Plane* frustumPlanes) {
  visibleChunks.clear();

  // TODO: refactore to fast index by offset
  for (u16 i = 0; i < chuncks.size(); i++) {
    if (chuncks[i]->getDistanceFromPlayerInChunks() > -1) {
      chuncks[i]->update(frustumPlanes);

      if (chuncks[i]->state == ChunkState::Loaded) {
        if (chuncks[i]->isVisible()) {
          if (!chuncks[i]->isDrawDataLoaded()) {
            chuncks[i]->loadDrawDataWithoutSorting();
          }
        } else {
          if (chuncks[i]->isDrawDataLoaded()) {
            chuncks[i]->clearDrawData();
          }
        }

        if (chuncks[i]->isDrawDataLoaded()) {
          visibleChunks.emplace_back(chuncks[i]);
        }
      }
    }
  }

  if (isTicksCounterAt(10) || isTimeToUpdateLight) {
    isTimeToUpdateLight = chuncksToUpdateLight.empty() == false;
    if (isTimeToUpdateLight) reloadLightDataAsync();
  }

  visibleChunks.shrink_to_fit();
}

void ChunckManager::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                             BlockManager* t_blockManager) {
  for (u16 i = 0; i < visibleChunks.size(); i++)
    visibleChunks[i]->renderer(t_renderer, stapip, t_blockManager);
  for (u16 i = 0; i < visibleChunks.size(); i++)
    visibleChunks[i]->rendererTransparentData(t_renderer, stapip,
                                              t_blockManager);
}

void ChunckManager::generateChunks() {
  u16 tempId = 0;

  for (size_t x = 0; x < OVERWORLD_MAX_DISTANCE; x += CHUNCK_SIZE) {
    for (size_t z = 0; z < OVERWORLD_MAX_DISTANCE; z += CHUNCK_SIZE) {
      for (size_t y = 0; y < OVERWORLD_MAX_HEIGH; y += CHUNCK_SIZE) {
        Vec4 tempMin = Vec4(x, y, z);
        Vec4 tempMax = Vec4(x + CHUNCK_SIZE, y + CHUNCK_SIZE, z + CHUNCK_SIZE);
        Chunck* tempChunck = new Chunck(tempMin, tempMax, tempId);
        tempChunck->init(terrain, worldLightModel);
        chuncks.emplace_back(tempChunck);

        tempId++;
      }
    }
  }

  tempId = 0;
  for (size_t x = 0; x < OVERWORLD_H_DISTANCE_IN_CHUNKS; x++) {
    for (size_t z = 0; z < OVERWORLD_H_DISTANCE_IN_CHUNKS; z++) {
      for (size_t y = 0; y < OVERWORLD_V_DISTANCE_IN_CHUNKS; y++) {
        auto origin = chuncks[tempId++];

        origin->topNeighbor = getChunkByOffset(Vec4(x, y + 1, z));
        origin->bottomNeighbor = getChunkByOffset(Vec4(x, y - 1, z));
        origin->backNeighbor = getChunkByOffset(Vec4(x, y, z + 1));
        origin->frontNeighbor = getChunkByOffset(Vec4(x, y, z - 1));
        origin->leftNeighbor = getChunkByOffset(Vec4(x + 1, y, z));
        origin->rightNeighbor = getChunkByOffset(Vec4(x - 1, y, z));
      }
    }
  }
};

Chunck* ChunckManager::getChunkById(const u16& id) {
  if (id < chuncks.size()) return chuncks[id];
  return nullptr;
};

void ChunckManager::enqueueChunksToReloadLight() {
  for (size_t i = 0; i < visibleChunks.size(); i++) {
    if (visibleChunks[i]->isDrawDataLoaded())
      chuncksToUpdateLight.push(visibleChunks[i]);
  }
}

void ChunckManager::reloadLightDataAsync() {
  auto chunk = chuncksToUpdateLight.front();
  chunk->reloadLightData();
  chuncksToUpdateLight.pop();
  return;
}

void ChunckManager::reloadLightData() {
  for (size_t i = 0; i < visibleChunks.size(); i++) {
    visibleChunks[i]->reloadLightData();
  }
  clearLightDataQueue();
}

// Needed to initiate light in all chunks. The visibleChunks will be available
// after the first update...
void ChunckManager::reloadLightDataOfAllChunks() {
  for (size_t i = 0; i < chuncks.size(); i++) {
    chuncks[i]->reloadLightData();
  }
  clearLightDataQueue();
}

const uint16_t ChunckManager::getChunkIdByPosition(
    const Vec4& chunkMinPosition) {
  const Vec4 pos = chunkMinPosition / CHUNCK_SIZE;
  const uint16_t row = pos.y;
  const uint16_t column = pos.z * OVERWORLD_V_DISTANCE_IN_CHUNKS;
  const uint16_t page = pos.x * OVERWORLD_PAGE_IN_CHUNKS;

  return page + column + row;
}

const uint16_t ChunckManager::getChunkIdByOffset(const Vec4& chunkMinOffset) {
  const Vec4 pos = chunkMinOffset;
  const uint16_t row = pos.y;
  const uint16_t column = pos.z * OVERWORLD_V_DISTANCE_IN_CHUNKS;
  const uint16_t page = pos.x * OVERWORLD_PAGE_IN_CHUNKS;

  return page + column + row;
}

Vec4 ChunckManager::getChunkPosById(const uint16_t& id) {
  const int x = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS);
  const int z = static_cast<int>((id - (x * OVERWORLD_PAGE_IN_CHUNKS)) /
                                 OVERWORLD_V_DISTANCE_IN_CHUNKS);
  const int y = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS);

  return Vec4(x, y, z) * CHUNCK_SIZE;
}

void ChunckManager::getChunkPosById(const uint16_t& id, Vec4* result) {
  result->x = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS) * CHUNCK_SIZE;
  result->z = static_cast<int>((id - (result->x * OVERWORLD_PAGE_IN_CHUNKS)) /
                               OVERWORLD_V_DISTANCE_IN_CHUNKS) *
              CHUNCK_SIZE;
  result->y = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS) * CHUNCK_SIZE;
}

Chunck* ChunckManager::getChunkByPosition(const Vec4& chunkMinPosition) {
  const uint16_t id = getChunkIdByPosition(chunkMinPosition);

  if (id < chuncks.size()) {
    return chuncks[id];
  } else {
    return nullptr;
  }
}

Chunck* ChunckManager::getChunkByOffset(const Vec4& chunkMinOffset) {
  const uint16_t id = getChunkIdByOffset(chunkMinOffset);

  if (id < chuncks.size()) {
    return chuncks[id];
  } else {
    return nullptr;
  }
}

Chunck* ChunckManager::getChunckByWorldPosition(const Vec4& pos) {
  Vec4 offset = (pos / DUBLE_BLOCK_SIZE) / CHUNCK_SIZE;
  Vec4 tempChunkMin =
      Vec4(std::floor(offset.x), std::floor(offset.y), std::floor(offset.z)) *
      CHUNCK_SIZE;

  return getChunkByPosition(tempChunkMin);
}

Chunck* ChunckManager::getChunkByBlockOffset(const Vec4& offset) {
  Vec4 _offset = offset / CHUNCK_SIZE;
  Vec4 tempChunkMin = Vec4(std::floor(_offset.x), std::floor(_offset.y),
                           std::floor(_offset.z)) *
                      CHUNCK_SIZE;

  return getChunkByPosition(tempChunkMin);
}
