#pragma once

#include <tamtypes.h>
#include <math/vec4.hpp>
#include "contants.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "managers/block_manager.hpp"

using Tyra::BBox;
using Tyra::Color;
using Tyra::M4x4;
using Tyra::McpipBlock;
using Tyra::Vec4;

/** Block 3D object class  */
class Block {
 public:
  u8 type = AIR_BLOCK;  // Init as air
  int index;            // Index at terrain;
  // Vec4 position;        // Index at terrain;

  // Block state
  u8 isTarget = 0;
  u8 isSolid = 0;
  u8 isBreakable = 0;
  u8 isSingleTexture = 1;
  float damage = 0;

  Vec4 minCorner;
  Vec4 maxCorner;

  M4x4 translation, rotation, scale;

  M4x4 model;
  Color color;
  Vec4 textureOffset;
  BBox* bbox = nullptr;

  // Distance to hit point when isTarget is true;
  float distance = 0.0f;

  Block(BlockInfo* blockInfo);
  ~Block();

  void updateModelMatrix();

  void setPosition(const Vec4& v);

  /** Get position from translation matrix */
  inline Vec4* getPosition() {
    return reinterpret_cast<Vec4*>(&translation.data[3 * 4]);
  }

 private:
  // Block props

  // Terrain generation params
  // float continentalness = 0.0;
  // float erosion = 0.0;
  // float peaks_and_valleys = 0.0;
  // float density = 0.0;

  // Biom's params
  // float temperature = 0.0;
  // float humidity = 0.0;
};
