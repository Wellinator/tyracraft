#pragma once

#include <tamtypes.h>
#include <math/vec4.hpp>
#include "contants.hpp"
#include "renderer/3d/mesh/mesh.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "managers/block_manager.hpp"

using Tyra::McpipBlock;
using Tyra::Mesh;
using Tyra::Vec4;

/** Block 3D object class  */
class Block : public McpipBlock {
 public:
  u8 type = AIR_BLOCK;  // Init as air
  int index;            // Index at terrain;
  Vec4 position;        // Index at terrain;

  // Block state
  u8 isTarget = 0;
  u8 isSolid = 0;
  u8 isEditable = 0;
  u8 visibility = 255;
  u8 isHidden = 1;
  u8 isSingleTexture = 1;

  Vec4 minCorner;
  Vec4 maxCorner;

  // Distance to hit point when isTarget is true;
  float distance = 0.0f;

  Block(BlockInfo* blockInfo);
  ~Block();

 private:
  // Block props

  // Terrain generation params
  float continentalness = 0.0;
  float erosion = 0.0;
  float peaks_and_valleys = 0.0;
  float density = 0.0;

  // Biom's params
  float temperature = 0.0;
  float humidity = 0.0;
};
