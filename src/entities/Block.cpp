#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "renderer/3d/bbox/bbox.hpp"

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::McpipBlock;

Block::Block(BlockInfo* blockInfo) {
  type = static_cast<Blocks>(blockInfo->blockId);
  isBreakable = blockInfo->_isBreakable;
  isCollidable = blockInfo->_isCollidable;
  hasTransparency = blockInfo->_isTransparent;
  isCrossed = blockInfo->_isCrossed;
  hardness = blockInfo->_hardness;

  for (size_t i = 0; i < 6; i++) {
    facesMapIndex[i] = blockInfo->_isSingle ? blockInfo->_facesMap[0]
                                            : blockInfo->_facesMap[i];
  }
}

Block::~Block() {
  if (bbox) delete bbox;
  bbox = NULL;
}
