#pragma once

#include <tamtypes.h>
#include <array>
#include <math/vec4.hpp>
#include "constants.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "models/block_info_model.hpp"

#define FRONT_VISIBLE 0x100000
#define BACK_VISIBLE 0x010000
#define LEFT_VISIBLE 0x001000
#define RIGHT_VISIBLE 0x000100
#define TOP_VISIBLE 0x000010
#define BOTTOM_VISIBLE 0x000001
#define HIDDEN_BLOCK 0x000000

enum class FACE_SIDE { FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM };

using Tyra::BBox;
using Tyra::Color;
using Tyra::M4x4;
using Tyra::McpipBlock;
using Tyra::Vec4;

/** Block 3D object class  */

/**
 *  TODO: compress Block data;
 *  boolean variable to u32 with bitshifting
 *  Vec4 to u32 compression
 */

class Block {
 public:
  Blocks type = Blocks::AIR_BLOCK;  // Init as air
  u32 index;                        // Index at terrain;
  Vec4 offset;                      // Terrain offset;
  u32 chunkId = 0;

  // Block state
  u8 isTarget = false;
  u8 isCollidable = false;
  u8 hasTransparency = false;
  u8 isBreakable = false;
  u8 isAtChunkBorder = false;
  u8 isCrossed = false;

  float damage = 0;

  Vec4 minCorner;
  Vec4 maxCorner;

  Vec4 position, rotation;

  BBox* bbox = nullptr;

  u32 visibleFaces = 0x000000;
  u8 visibleFacesCount = 0;

  /**
   * Order: Top, Bottom, Left, Right, Front, Back
   * @param facesMapIndex 6 length array pair of integers col, row
   */
  std::array<u8, 6> facesMapIndex = {0, 0, 0, 0, 0, 0};

  // Distance to hit point when isTarget is true;
  float distance = 0.0f;
  Vec4 hitPosition;

  Block(BlockInfo* blockInfo);
  ~Block();

  inline const bool isFrontFaceVisible() {
    return (visibleFaces & FRONT_VISIBLE) == FRONT_VISIBLE;
  };

  inline const bool isBackFaceVisible() {
    return (visibleFaces & BACK_VISIBLE) == BACK_VISIBLE;
  };

  inline const bool isLeftFaceVisible() {
    return (visibleFaces & LEFT_VISIBLE) == LEFT_VISIBLE;
  };

  inline const bool isRightFaceVisible() {
    return (visibleFaces & RIGHT_VISIBLE) == RIGHT_VISIBLE;
  };

  inline const bool isTopFaceVisible() {
    return (visibleFaces & TOP_VISIBLE) == TOP_VISIBLE;
  };

  inline const bool isBottomFaceVisible() {
    return (visibleFaces & BOTTOM_VISIBLE) == BOTTOM_VISIBLE;
  };

  inline const float getHardness() { return hardness; }

  Color baseColor;

 private:
  // Block props

  float hardness = 0;

  // Terrain generation params
  // float continentalness = 0.0;
  // float erosion = 0.0;
  // float peaks_and_valleys = 0.0;
  // float density = 0.0;

  // Biom's params
  // float temperature = 0.0;
  // float humidity = 0.0;
};
