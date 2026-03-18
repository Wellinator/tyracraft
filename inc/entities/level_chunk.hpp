#pragma once

#include <stdint.h>
#include "constants.hpp"

/**
 * @brief Represents a single 16x16x16 section of world data.
 * This is the atomic unit for dynamic loading and memory management.
 */
class LevelSection {
 public:
  LevelSection();
  ~LevelSection();

  uint8_t blocks[CHUNK_LENGTH];
  uint8_t lightData[CHUNK_LENGTH];
  uint8_t metaData[CHUNK_LENGTH];

  bool isAirOnly;
  
  void clear();
};

/**
 * @brief Represents a 16x16xWorldHeight column of LevelSections.
 */
class LevelChunk {
 public:
  LevelChunk(int x, int z);
  ~LevelChunk();

  int x, z;
  
  // Vertical sections (e.g., 6 sections for 96 blocks height)
  LevelSection* sections[OVERWORLD_V_DISTANCE_IN_CHUNKS];

  // Bitset to track which sections are loaded
  uint8_t loadedSectionsMask;

  bool isDirty;
  bool terrainPopulated;

  LevelSection* getSection(int y);
  void allocateSection(int y);
  void deallocateSection(int y);
};
