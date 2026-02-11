#pragma once

#include <tamtypes.h>
#include <queue>
#include <unordered_set>
#include "constants.hpp"
#include "entities/level.hpp"
#include "models/bfs_node.hpp"

class ChunkManager;
class WorldLightPropagation;
struct Chunk;

/**
 * @brief Manages water and lava BFS propagation logic.
 *
 * Extracted from World to isolate all liquid-related flood-fill logic.
 * Dependencies: Level*, ChunkManager*, WorldLightPropagation* (lava emits
 * light).
 */
class WorldLiquidPropagation {
 public:
  WorldLiquidPropagation();
  ~WorldLiquidPropagation();

  void init(Level* level, ChunkManager* chunkManager,
            WorldLightPropagation* lightPropagation);

  /** @brief Scans entire map and seeds initial liquid expansion queues. */
  void initLiquidExpansion();

  /**
   * @brief Propagate all liquids until queues are drained.
   * Called once during world loading.
   */
  void propagateAll();

  /** @brief Tick-based water propagation step. */
  void updateLiquidWater();

  /** @brief Tick-based lava propagation step. */
  void updateLiquidLava();

  /** @brief Rebuild chunks that were modified by liquid propagation. */
  void updateChunksAffectedByLiquidPropagation();

  /** @brief Returns true if there are chunks affected by liquid. */
  inline bool hasAffectedChunks() const {
    return affectedChunksIdByLiquidPropagation.size() > 0;
  }

  /**
   * @brief Check neighbors of a removed block for liquid re-propagation.
   */
  void checkLiquidPropagation(uint16_t x, uint16_t y, uint16_t z);

  void addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level);
  void addLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level,
                 u8 orientation);
  void removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type);
  void removeLiquid(uint16_t x, uint16_t y, uint16_t z, u8 type, u8 level);

 private:
  Level* pLevel = nullptr;
  ChunkManager* pChunkManager = nullptr;
  WorldLightPropagation* pLightPropagation = nullptr;

  // Water BFS queues
  std::queue<BfsNode> waterBfsQueue;
  std::queue<BfsNode> waterRemovalBfsQueue;

  // Lava BFS queues
  std::queue<BfsNode> lavaBfsQueue;
  std::queue<BfsNode> lavaRemovalBfsQueue;

  // Chunks dirtied by liquid propagation (rebuilt each frame)
  std::unordered_set<Chunk*> affectedChunksIdByLiquidPropagation;

  // Water propagation
  void propagateWaterRemovalQueue();
  void propagateWaterAddQueue();

  // Lava propagation
  void propagateLavaRemovalQueue();
  void propagateLavaAddQueue();

  void floodFillLiquidAdd(uint16_t x, uint16_t y, uint16_t z, u8 type,
                          u8 nextLevel, u8 orientation);
  void floodFillLiquidRemove(uint16_t x, uint16_t y, uint16_t z, u8 type,
                             u8 level);

  const s8 getNextLavaLevel(const s8 currentLevel);
  u8 canPropagateLiquid(uint16_t x, uint16_t y, uint16_t z);
};
