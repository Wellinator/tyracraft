#include "managers/chunck_manager.hpp"
#include "math/plane.hpp"

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Vec4;

ChunckManager::ChunckManager() {}

ChunckManager::~ChunckManager() {
  for (u16 i = 0; i < this->chuncks.size(); i++) {
    delete chuncks[i];
    chuncks[i] = NULL;
  }
  this->chuncks.clear();
}

void ChunckManager::init() { this->generateChunks(); }

void ChunckManager::update(const Plane* frustumPlanes,
                           const Vec4& currentPlayerPos,
                           WorldLightModel* worldLightModel) {
  this->visibleChunks.clear();
  for (u16 i = 0; i < chuncks.size(); i++) {
    chuncks[i]->update(frustumPlanes, currentPlayerPos, worldLightModel);
    if (chuncks[i]->isVisible() && chuncks[i]->state == ChunkState::Loaded)
      this->visibleChunks.push_back(chuncks[i]);
  }
}

void ChunckManager::renderer(Renderer* t_renderer, StaticPipeline* stapip,
                             BlockManager* t_blockManager) {
  for (u16 i = 0; i < this->chuncks.size(); i++)
    chuncks[i]->renderer(t_renderer, stapip, t_blockManager);
}

void ChunckManager::generateChunks() {
  // TODO: create only the chuncks that'll be rendered
  u16 tempId = 1;

  for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE;
       x += CHUNCK_SIZE) {
    for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE;
         z += CHUNCK_SIZE) {
      Vec4 tempMin = Vec4(x, OVERWORLD_MIN_HEIGH, z);
      Vec4 tempMax =
          Vec4(x + CHUNCK_SIZE, OVERWORLD_MAX_HEIGH, z + CHUNCK_SIZE);
      Chunck* tempChunck = new Chunck(tempMin, tempMax, tempId);
      this->chuncks.push_back(tempChunck);
      tempId++;
    }
  }
};

Chunck* ChunckManager::getChunckByPosition(const Vec4& position) {
  for (u16 i = 0; i < this->chuncks.size(); i++)
    if (position.collidesBox(*chuncks[i]->minOffset * DUBLE_BLOCK_SIZE,
                             *chuncks[i]->maxOffset * DUBLE_BLOCK_SIZE)) {
      return chuncks[i];
    }
  return nullptr;
};

Chunck* ChunckManager::getChunckById(const u16 id) {
  for (u16 i = 0; i < this->chuncks.size(); i++)
    if (chuncks[i]->id == id) return chuncks[i];
  return nullptr;
};

u8 ChunckManager::isChunkVisible(Chunck* chunk) { return chunk->isVisible(); }

std::vector<Chunck*> ChunckManager::getVisibleChunks() {
  return this->visibleChunks;
}