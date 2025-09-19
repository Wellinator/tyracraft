#include "managers/chunk_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math/plane.hpp"
#include "debug.hpp"
#include <algorithm>

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Vec4;

ChunkManager::ChunkManager() : Singleton<ChunkManager>() {}

ChunkManager::~ChunkManager() {
  for (u16 i = 0; i < chunks.size(); i++) {
    delete chunks[i];
    chunks[i] = NULL;
  }
  chunks.clear();
  chunks.shrink_to_fit();

  visibleChunks.clear();
  visibleChunks.shrink_to_fit();

  loadedChunks.clear();
  loadedChunks.shrink_to_fit();
}

void ChunkManager::init(WorldLightModel* t_worldLightModel, Level* level) {
  worldLightModel = t_worldLightModel;
  pLevel = level;
  this->generateChunks();
}

void ChunkManager::clearAllChunks() {
  for (u16 i = 0; i < chunks.size(); i++) chunks[i]->clear();
}

void ChunkManager::updateLoadedChunks() {
  loadedChunks.clear();
  for (u16 i = 0; i < chunks.size(); i++) {
    if (chunks[i]->isLoaded() == false) continue;
    loadedChunks.emplace_back(chunks[i]);
  }
}

void ChunkManager::update(const Plane* frustumPlanes, Vec4* camPos) {
  visibleChunks.clear();

  // TODO: refactore to fast index by offset
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    Chunk* chk = loadedChunks[i];
    if (chk->isLoaded()) {
      chk->setCamPosition(camPos);
      chk->update(frustumPlanes);

      if (chk->isVisible()) visibleChunks.emplace_back(chk);
    }
  }
}

void ChunkManager::tick() {
  if (isTicksCounterAt(2)) {
    if (chunksToUpdateLight.empty() == false) reloadLightDataAsync();
  }

  // This tick time must be in sync with World::updateLightModel()
  if (isTicksCounterAt(250) && chunksToUpdateLight.empty() == false) {
    enqueueChunksToReloadLight();
  }

  for (size_t i = 0; i < chunks.size(); i++) {
    chunks[i]->tick();
  }
}

void ChunkManager::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  for (u16 i = 0; i < visibleChunks.size(); i++)
    visibleChunks[i]->renderer(t_renderer, stapip);
  for (u16 i = 0; i < visibleChunks.size(); i++)
    visibleChunks[i]->rendererTransparentData(t_renderer, stapip);

#ifdef DEBUG_MODE
  if (g_debug_menu.showChunkBorders) {
    for (u16 i = 0; i < visibleChunks.size(); i++) {
      t_renderer->renderer3D.utility.drawBBox(*visibleChunks[i]->bbox,
                                              Color(50, 50, 200));
    }
  }
#endif  // end if DEBUG_MODE
}

void ChunkManager::rendererOpaque(Renderer* t_renderer,
                                  StaticPipeline* stapip) {
  for (u16 i = 0; i < visibleChunks.size(); i++) {
    visibleChunks[i]->renderer(t_renderer, stapip);

#ifdef DEBUG_MODE
    if (g_debug_menu.showChunkBorders) {
      t_renderer->renderer3D.utility.drawBBox(*visibleChunks[i]->bbox,
                                              Color(50, 50, 200));
    }
#endif  // end if DEBUG_MODE
  }
}

void ChunkManager::rendererTransparent(Renderer* t_renderer,
                                       StaticPipeline* stapip) {
  for (u16 i = 0; i < visibleChunks.size(); i++) {
    visibleChunks[i]->rendererTransparentData(t_renderer, stapip);
#ifdef DEBUG_MODE
    if (g_debug_menu.showChunkBorders) {
      t_renderer->renderer3D.utility.drawBBox(*visibleChunks[i]->bbox,
                                              Color(50, 50, 200));
    }
#endif  // end if DEBUG_MODE
  }
}

void ChunkManager::generateChunks() {
  u16 tempId = 0;

  for (size_t x = 0; x < OVERWORLD_MAX_DISTANCE; x += CHUNK_SIZE) {
    for (size_t z = 0; z < OVERWORLD_MAX_DISTANCE; z += CHUNK_SIZE) {
      for (size_t y = 0; y < OVERWORLD_MAX_HEIGH; y += CHUNK_SIZE) {
        Vec4 tempMin = Vec4(x, y, z);
        Vec4 tempMax = Vec4(x + CHUNK_SIZE, y + CHUNK_SIZE, z + CHUNK_SIZE);
        Chunk* tempChunk = new Chunk(tempMin, tempMax, tempId);
        tempChunk->init(pLevel, worldLightModel);
        chunks.emplace_back(tempChunk);

        tempId++;
      }
    }
  }
};

Chunk* ChunkManager::getChunkById(const u16& id) {
  if (id < chunks.size()) return chunks[id];
  return nullptr;
};

void ChunkManager::enqueueChunksToReloadLight() {
  for (size_t i = 0; i < chunks.size(); i++) {
    if (chunks[i]->isLoaded()) chunksToUpdateLight.push(chunks[i]);
  }
}

void ChunkManager::reloadLightDataAsync() {
  auto chunk = chunksToUpdateLight.front();
  chunk->reloadLightData();
  chunksToUpdateLight.pop();
  return;
}

void ChunkManager::reloadLightData() {
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    loadedChunks[i]->reloadLightData();
  }
  clearLightDataQueue();
}

// Needed to initiate light in all chunks. The visibleChunks will be available
// after the first update...
void ChunkManager::reloadLightDataOfAllChunks() {
  for (size_t i = 0; i < chunks.size(); i++) {
    if (chunks[i]->isLoaded()) chunks[i]->reloadLightData();
  }
  clearLightDataQueue();
}

// void ChunkManager::sortDrawDataFromCamPos(const Vec4& cameraPos) {
//   for (size_t i = 0; i < visibleChunks.size(); i++) {
//     visibleChunks[i]->sortTransParentDrawData(cameraPos);
//   }
// }

const uint16_t ChunkManager::getChunkIdByPosition(
    const Vec4& chunkMinPosition) {
  const Vec4 offset = chunkMinPosition / CHUNK_SIZE;
  return getChunkIdByOffset(offset);
}

const uint16_t ChunkManager::getChunkIdByOffset(const Vec4& chunkMinOffset) {
  const Vec4 pos = chunkMinOffset;
  const uint16_t row = pos.y;
  const uint16_t column = pos.z * OVERWORLD_V_DISTANCE_IN_CHUNKS;
  const uint16_t page = pos.x * OVERWORLD_PAGE_IN_CHUNKS;

  return page + column + row;
}

Vec4 ChunkManager::getChunkPosById(const uint16_t& id) {
  const int x = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS);
  const int z = static_cast<int>((id - (x * OVERWORLD_PAGE_IN_CHUNKS)) /
                                 OVERWORLD_V_DISTANCE_IN_CHUNKS);
  const int y = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS);

  return Vec4(x, y, z) * CHUNK_SIZE;
}

void ChunkManager::getChunkPosById(const uint16_t& id, Vec4* result) {
  result->x = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS) * CHUNK_SIZE;
  result->z = static_cast<int>((id - (result->x * OVERWORLD_PAGE_IN_CHUNKS)) /
                               OVERWORLD_V_DISTANCE_IN_CHUNKS) *
              CHUNK_SIZE;
  result->y = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS) * CHUNK_SIZE;
}

Chunk* ChunkManager::getChunkByPosition(const Vec4& chunkMinPosition) {
  const uint16_t id = getChunkIdByPosition(chunkMinPosition);

  if (id < chunks.size()) {
    return chunks[id];
  } else {
    return nullptr;
  }
}

Chunk* ChunkManager::getChunkByOffset(const Vec4& chunkMinOffset) {
  const uint16_t id = getChunkIdByOffset(chunkMinOffset);

  if (id < chunks.size()) {
    return chunks[id];
  } else {
    return nullptr;
  }
}

Chunk* ChunkManager::getChunkByWorldPosition(const Vec4& pos) {
  Vec4 offset = (pos / DOUBLE_BLOCK_SIZE) / CHUNK_SIZE;
  Vec4 tempChunkMin =
      Vec4(std::floor(offset.x), std::floor(offset.y), std::floor(offset.z)) *
      CHUNK_SIZE;

  return getChunkByPosition(tempChunkMin);
}

Chunk* ChunkManager::getChunkByBlockOffset(const Vec4& offset) {
  Vec4 _offset = offset / CHUNK_SIZE;
  Vec4 tempChunkMin = Vec4(std::floor(_offset.x), std::floor(_offset.y),
                           std::floor(_offset.z)) *
                      CHUNK_SIZE;

  return getChunkByPosition(tempChunkMin);
}

int ChunkManager::getHeightAtOffset(const Vec4& offset) {
  int y = pLevel->map.height - 1;

  while (y >= 0) {
    if (pLevel->GetBlockFromMap(offset.x, y - 1, offset.z) !=
        (int)Blocks::AIR_BLOCK)
      return y;
    y--;
  }

  return 0;
}

float ChunkManager::getHeightAtPosition(const Vec4& position) {
  const Vec4 offset = pLevel->worldPosToOffset(position);
  return getHeightAtOffset(offset) * DOUBLE_BLOCK_SIZE;
}
