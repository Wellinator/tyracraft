#pragma once

#include "chunck.hpp"
#include "managers/mob/mob_manager.hpp"

class SpawnRule {
 public:
  SpawnRule();
  ~SpawnRule();

  int spawnChance;

  /**
   * @brief Spawn mob after chunk generation
   */
  virtual void GenerateMobs(Chunck* pChunk, MobManager* pMobManager){};

  /**
   * @brief Spawn mob
   */
  virtual void SpawnMobs(Chunck* pChunk, MobManager* pMobManager){};
};
