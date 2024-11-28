#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "renderer/3d/bbox/bbox.hpp"

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::McpipBlock;

Block::Block(BlockInfo* blockInfo) : Entity(nullptr, EntityType::Block) {
  pBlockInfo = blockInfo;
  collidable = pBlockInfo->_isCollidable;
}

Block::~Block() {
  if (bbox) delete bbox;
  bbox = nullptr;
}

Blocks Block::getType() { return static_cast<Blocks>(pBlockInfo->blockId); }

float Block::getHardness() { return pBlockInfo->_hardness; }

/**
 * Order: Top, Bottom, Left, Right, Back, Front
 * @param facesMapIndex 6 length array of texture index
 */
std::array<u8, 6>* Block::getFacesMap() { return &pBlockInfo->_facesMap; };

u8 Block::isBreakable() { return pBlockInfo->_isBreakable; }

u8 Block::isCollidable() { return pBlockInfo->_isCollidable; }

u8 Block::hasTransparency() { return pBlockInfo->_isTransparent; }

u8 Block::isCrossed() { return pBlockInfo->_isCrossed; }