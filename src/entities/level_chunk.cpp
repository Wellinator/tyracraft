#include "entities/level_chunk.hpp"
#include <string.h>

LevelSection::LevelSection() {
  clear();
}

LevelSection::~LevelSection() {}

void LevelSection::clear() {
  memset(blocks, (uint8_t)Blocks::AIR_BLOCK, CHUNK_LENGTH);
  memset(lightData, 0, CHUNK_LENGTH);
  memset(metaData, 0, CHUNK_LENGTH);
  isAirOnly = true;
}

LevelChunk::LevelChunk(int x, int z) : x(x), z(z) {
  loadedSectionsMask = 0;
  isDirty = false;
  terrainPopulated = false;
  for (int i = 0; i < OVERWORLD_V_DISTANCE_IN_CHUNKS; i++) {
    sections[i] = nullptr;
  }
}

LevelChunk::~LevelChunk() {
  for (int i = 0; i < OVERWORLD_V_DISTANCE_IN_CHUNKS; i++) {
    deallocateSection(i);
  }
}

LevelSection* LevelChunk::getSection(int y) {
  if (y < 0 || y >= OVERWORLD_V_DISTANCE_IN_CHUNKS) return nullptr;
  return sections[y];
}

void LevelChunk::allocateSection(int y) {
  if (y < 0 || y >= OVERWORLD_V_DISTANCE_IN_CHUNKS) return;
  if (sections[y] == nullptr) {
    sections[y] = new LevelSection();
    loadedSectionsMask |= (1 << y);
  }
}

void LevelChunk::deallocateSection(int y) {
  if (y < 0 || y >= OVERWORLD_V_DISTANCE_IN_CHUNKS) return;
  if (sections[y] != nullptr) {
    delete sections[y];
    sections[y] = nullptr;
    loadedSectionsMask &= ~(1 << y);
  }
}
