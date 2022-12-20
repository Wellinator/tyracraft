#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "renderer/3d/bbox/bbox.hpp"

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::McpipBlock;

Block::Block(BlockInfo* blockInfo) {
  this->type = static_cast<Blocks>(blockInfo->blockId);
  this->isBreakable = blockInfo->_isBreakable;
  this->isSolid = blockInfo->_isSolid;
  this->model.identity();
  this->scale.identity();
  this->translation.identity();
  this->rotation.identity();

  blockInfo->_isSingle
      ? setSingleFaces(blockInfo->_facesMap[0], blockInfo->_facesMap[1])
      : setMultipleFaces(blockInfo->_facesMap.data());
}

Block::~Block() {
  if (bbox) delete bbox;
  bbox = NULL;
}

void Block::updateModelMatrix() { model = translation * rotation * scale; }

void Block::setPosition(const Vec4& v) {
  TYRA_ASSERT(v.w == 1.0F, "Vec4 must be homogeneous");
  reinterpret_cast<Vec4*>(&translation.data[3 * 4])->set(v);
}

void Block::setSingleFaces(const u8& col, const u8& row) {
  for (size_t i = 0; i < 12; i += 2) {
    facesMap[i] = col;
    facesMap[i + 1] = row;
  }
}

void Block::setMultipleFaces(const u8* uv) {
  for (size_t i = 0; i < 12; i += 2) {
    facesMap[i] = uv[i];
    facesMap[i + 1] = uv[i + 1];
  }
}