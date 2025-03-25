#pragma once

#include "chunk.hpp"
#include "managers/mob/mob_manager.hpp"

class SpawnRule {
 public:
  SpawnRule();
  ~SpawnRule();

  int spawnChance;

  /**
   * @brief Spawn mob after chunk generation
   */
  virtual void GenerateMobs(Chunk* pChunk, MobManager* pMobManager){};

  /**
   * @brief Spawn mob
   */
  virtual void SpawnMobs(Chunk* pChunk, MobManager* pMobManager){};
};
