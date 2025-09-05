#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "renderer/3d/bbox/bbox.hpp"

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::McpipBlock;

Block::Block() : Entity(nullptr, EntityType::Block) {
  // Initialize packed structure
  packed = {};  // Zero-initialize all packed fields
}

Block::~Block() {
  if (isTemplate) {
    TYRA_TRAP("Trying to delete a block template!");
  }

  if (bbox) delete bbox;
  bbox = nullptr;
}