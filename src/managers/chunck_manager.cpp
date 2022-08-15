#include "managers/chunck_manager.hpp"

ChunckManager::ChunckManager() {}
ChunckManager::~ChunckManager() {}

void ChunckManager::init(BlockManager* t_blockManager) {
  this->t_blockManager = t_blockManager;
  this->generateChunks();
}

void ChunckManager::generateChunks() {
  u16 tempId = 1;
  for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE;
       x += CHUNCK_SIZE) {
    for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE;
         z += CHUNCK_SIZE) {
      Vec4 tempMin = Vec4(x, OVERWORLD_MIN_HEIGH, z);
      Vec4 tempMax =
          Vec4(x + CHUNCK_SIZE, OVERWORLD_MAX_HEIGH, z + CHUNCK_SIZE);
      Chunck* tempChunck =
          new Chunck(this->t_blockManager, tempMin, tempMax, tempId);
      this->chuncks.push_back(tempChunck);
      tempId++;
    }
  }
};
