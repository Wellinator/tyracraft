#pragma once

#include <memory>
#include <array>
#include "constants.hpp"
#include "singleton.hpp"
#include "entities/Block.hpp"

/**
 * Static repository for block instances.
 * Maintains the same order as the original BlockInfoRepository for
 * compatibility. Each block type is created once and reused as a template.
 */
class StaticBlockRepository : public Singleton<StaticBlockRepository> {
 public:
  StaticBlockRepository();

  /**
   * Get a block template by type.
   * Returns a reference to the static block instance.
   */
  Block* getBlockTemplate(Blocks blockType);
  Block* getBlockTemplate(u8 blockId);

  /**
   * Create a new block instance of the specified type.
   * This creates a copy of the template for use in the world.
   */
  Block* createBlock(Blocks blockType);

  /**
   * Check if a block type is transparent (via template vtable — prefer
   * isBlockTransparentFast() for hot paths inside chunk build loops).
   */
  bool isBlockTransparent(Blocks blockType);

  /**
   * O(1) transparency lookup — no singleton chain, no virtual dispatch.
   * Safe to call with any u8 block ID including VOID (0) and AIR (1).
   *   VOID (0)      → 0   (opaque — treat as solid boundary)
   *   AIR_BLOCK (1) → 1   (transparent)
   *   others        → 0 or 1 from the block template
   *
   * Populated once in initializeBlocks().  Read-only after construction.
   */
  static u8 s_transparencyTable[256];

 private:
  void initializeBlocks();
  void buildTransparencyTable();
};
