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
  ~StaticBlockRepository() = default;

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
   * Check if a block type is transparent
   */
  bool isBlockTransparent(Blocks blockType);

  /**
   * Get the singleton instance of the StaticBlockRepository.
   */
  void initializeBlocks();
};
