#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"

using Tyra::McpipBlock;

Block::Block(BlockInfo* blockInfo) : McpipBlock() {
  this->type = blockInfo->blockId;
  this->isSingleTexture = blockInfo->_isSingle;
  this->textureOffset = Vec4(blockInfo->_texOffssetX, blockInfo->_texOffssetY, 0.0F, 1.0F);
}

Block::~Block() {}
