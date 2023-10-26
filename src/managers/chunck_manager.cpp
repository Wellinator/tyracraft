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

void ChunckManager::update(const Plane* frustumPlanes,
                           const Vec4& currentPlayerPos) {
  visibleChunks.clear();
  nearByChunks.clear();

  // TODO: refactore to fast index by offset
  for (u16 i = 0; i < chuncks.size(); i++) {
    if (chuncks[i]->getDistanceFromPlayerInChunks() > -1) {
      chuncks[i]->update(frustumPlanes);

      if (chuncks[i]->getDistanceFromPlayerInChunks() <= 2) {
        nearByChunks.emplace_back(chuncks[i]);
      }

      if (chuncks[i]->state == ChunkState::Loaded &&
          chuncks[i]->isDrawDataLoaded()) {
        visibleChunks.emplace_back(chuncks[i]);
      }
    }
  }

  if (isTicksCounterAt(10) || isTimeToUpdateLight) {
    isTimeToUpdateLight = chuncksToUpdateLight.empty() == false;
    if (isTimeToUpdateLight) reloadLightDataAsync();
  }
}

void ChunckManager::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                             BlockManager* t_blockManager) {
  for (u16 i = 0; i < chuncks.size(); i++)
    if (chuncks[i]->isVisible())
      chuncks[i]->renderer(t_renderer, stapip, t_blockManager);
}

void ChunckManager::generateChunks() {
  u16 tempId = 1;

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
};

Chunck* ChunckManager::getChunckByPosition(const Vec4& position) {
  for (u16 i = 0; i < chuncks.size(); i++)
    if (position.collidesBox(*chuncks[i]->minOffset * DUBLE_BLOCK_SIZE,
                             *chuncks[i]->maxOffset * DUBLE_BLOCK_SIZE)) {
      return chuncks[i];
    }
  return nullptr;
};

Chunck* ChunckManager::getChunckByOffset(const Vec4& offset) {
  for (size_t i = 0; i < chuncks.size(); i++)
    if (chuncks[i]->isVisible() && offset.x >= chuncks[i]->minOffset->x &&
        offset.x < chuncks[i]->maxOffset->x &&
        offset.y >= chuncks[i]->minOffset->y &&
        offset.y < chuncks[i]->maxOffset->y &&
        offset.z >= chuncks[i]->minOffset->z &&
        offset.z < chuncks[i]->maxOffset->z) {
      return chuncks[i];
    }
  return nullptr;
};

Chunck* ChunckManager::getChunckById(const u16& id) { return chuncks[id - 1]; };

u8 ChunckManager::isChunkVisible(Chunck* chunk) { return chunk->isVisible(); }

std::vector<Chunck*>* ChunckManager::getVisibleChunks() {
  return &visibleChunks;
}

std::vector<Chunck*>* ChunckManager::getNearByChunks() { return &nearByChunks; }

void ChunckManager::sortChunkByPlayerPosition(Vec4* playerPosition) {
  std::sort(chuncks.begin(), chuncks.end(),
            [playerPosition](const Chunck* a, const Chunck* b) {
              const Vec4 dest = *playerPosition / DUBLE_BLOCK_SIZE;
              const float distanceA = (*a->center).distanceTo(dest);
              const float distanceB = (*b->center).distanceTo(dest);
              return distanceA >= distanceB;
            });
}

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

u32 ChunckManager::getIndexByOffset(u16 x, u16 y, u16 z) {
  return (y * OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD) +
         (z * OVERWORLD_H_DISTANCE_IN_CHUNKS) + x;
}
