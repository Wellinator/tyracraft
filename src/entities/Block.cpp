#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "renderer/3d/bbox/bbox.hpp"

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::McpipBlock;

Block::Block(BlockInfo* blockInfo) {
  this->type = static_cast<Blocks>(blockInfo->blockId);
  this->isBreakable = blockInfo->_isBreakable;
  this->isCollidable = blockInfo->_isCollidable;
  this->hasTransparency = blockInfo->_isTransparent;
  this->isCrossed = blockInfo->_isCrossed;
  this->hardness = blockInfo->_hardness;

  for (size_t i = 0; i < 6; i++) {
    facesMapIndex[i] = blockInfo->_isSingle ? blockInfo->_facesMap[0]
                                            : blockInfo->_facesMap[i];
  }
}

Block::~Block() {
  if (bbox) delete bbox;
  bbox = NULL;
}
