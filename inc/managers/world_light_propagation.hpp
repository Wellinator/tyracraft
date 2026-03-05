#pragma once

#include <tamtypes.h>
#include <queue>
#include "constants.hpp"
#include "entities/level.hpp"
#include "models/bfs_node.hpp"

class BlockManager;

/**
 * @brief Manages sunlight and block-light BFS propagation.
 *
 * Extracted from World to isolate all light-related flood-fill logic.
 * Dependencies: Level* (map data), BlockManager* (block light values).
 */
class WorldLightPropagation {
 public:
  WorldLightPropagation();
  ~WorldLightPropagation();

  void init(Level* level);

  // --- Sunlight ---
  void initSunLight(uint32_t tick);
  void addSunLight(uint16_t x, uint16_t y, uint16_t z);
  void addSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
  void removeSunLight(uint16_t x, uint16_t y, uint16_t z);
  void removeSunLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
  void updateSunlight();
  void checkSunLightAt(uint16_t x, uint16_t y, uint16_t z);

  /** @brief Budget-bounded sunlight propagation. Returns true when complete. */
  bool updateSunlightBudgeted(u32 budgetCycles);
  /** @brief Budget-bounded block light propagation. Returns true when complete. */
  bool updateBlockLightsBudgeted(u32 budgetCycles);

  // --- Block Light ---
  void initBlockLight(BlockManager* blockManager);
  void addBlockLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
  void removeLight(uint16_t x, uint16_t y, uint16_t z);
  void removeLight(uint16_t x, uint16_t y, uint16_t z, u8 lightLevel);
  void updateBlockLights();

  /** @brief Check if any light propagation is pending. */
  inline bool isAnyPropagationActive() const {
    return !sunlightBfsQueue.empty() || !sunlightRemovalBfsQueue.empty() ||
           !lightBfsQueue.empty() || !lightRemovalBfsQueue.empty();
  }

  /** @brief Returns true if the block type allows light to pass through. */
  inline bool isTransparent(Blocks block) const {
    return block == Blocks::AIR_BLOCK || block == Blocks::WATER_BLOCK ||
           block == Blocks::GRASS || block == Blocks::POPPY_FLOWER ||
           block == Blocks::DANDELION_FLOWER || block == Blocks::TORCH ||
           block == Blocks::GLASS_BLOCK || block == Blocks::OAK_LEAVES_BLOCK ||
           block == Blocks::BIRCH_LEAVES_BLOCK ||
           // Slabs
           ((u8)block >= (u8)Blocks::STONE_SLAB &&
            (u8)block <= (u8)Blocks::MOSSY_STONE_BRICKS_SLAB);
  };

 private:
  Level* pLevel = nullptr;

  // Sunlight BFS queues
  std::queue<BfsNode> sunlightBfsQueue;
  std::queue<BfsNode> sunlightRemovalBfsQueue;

  // Block light BFS queues
  std::queue<BfsNode> lightBfsQueue;
  std::queue<BfsNode> lightRemovalBfsQueue;

  // Sunlight BFS helpers
  void propagateSunLightAddBFSQueue();
  void propagateSunlightRemovalQueue();
  bool propagateSunLightAddBFSQueueBudgeted(u32& remainingCycles);
  bool propagateSunlightRemovalQueueBudgeted(u32& remainingCycles);
  void floodFillSunlightAdd(uint16_t x, uint16_t y, uint16_t z,
                            u8 nextLightValue);
  void floodFillSunlightRemove(uint16_t x, uint16_t y, uint16_t z,
                               u8 lightLevel);

  // Block light BFS helpers
  void propagateLightRemovalQueue();
  void propagateLightAddQueue();
  bool propagateLightRemovalQueueBudgeted(u32& remainingCycles);
  bool propagateLightAddQueueBudgeted(u32& remainingCycles);
  void floodFillLightAdd(uint16_t x, uint16_t y, uint16_t z,
                         u8 nextLightValue);
  void floodFillLightRemove(uint16_t x, uint16_t y, uint16_t z,
                            u8 lightLevel);
};
