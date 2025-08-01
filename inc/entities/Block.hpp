#pragma once

#include <tamtypes.h>
#include <array>
#include <math/vec4.hpp>
#include "constants.hpp"
#include "entities/entity.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"

#define FRONT_VISIBLE 0b100000
#define BACK_VISIBLE 0b010000
#define LEFT_VISIBLE 0b001000
#define RIGHT_VISIBLE 0b000100
#define TOP_VISIBLE 0b000010
#define BOTTOM_VISIBLE 0b000001
#define HIDDEN_BLOCK 0b000000

enum class FACE_SIDE { FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM };

enum class BlockFace {
  FRONT = FRONT_VISIBLE,
  BACK = BACK_VISIBLE,
  LEFT = LEFT_VISIBLE,
  RIGHT = RIGHT_VISIBLE,
  TOP = TOP_VISIBLE,
  BOTTOM = BOTTOM_VISIBLE
};

using Tyra::Color;
using Tyra::M4x4;
using Tyra::Vec4;

// Helper macro to reduce code duplication for clone methods
#define IMPLEMENT_BLOCK_CLONE(ClassName) \
  Block* clone() override { \
    ClassName* newBlock = new ClassName(); \
    newBlock->packed = packed; \
    newBlock->offset = offset; \
    newBlock->model = model; \
    newBlock->compressedHitPosition = compressedHitPosition; \
    newBlock->damage = damage; \
    newBlock->distance = distance; \
    newBlock->baseColor = baseColor; \
    return newBlock; \
  }

/** Block 3D object class  */

/**
 *  Block data compression implemented:
 *  - Packed boolean flags into bitfields
 *  - Compressed Vec4 hitPosition using 16-bit integers
 *  - Packed multiple small integers together
 *  - Memory layout optimized for cache efficiency
 */

class Block : public Entity {
 protected:
  Block();

 public:
  virtual ~Block();
  virtual Block* clone() = 0;

  // Core block data - keep these first for cache efficiency
  u32 index;  // Index at terrain;

  // Packed data structure to reduce memory footprint
  struct {
    u16 chunkId;
    u16 localIndex;
    u16 drawDataIndex;
    u8 drawDataLength;
    u8 visibleFaces;           // 6 bits for faces, 2 bits unused
    u8 visibleFacesCount : 4;  // Max 6 faces, only need 4 bits
    u8 isTarget : 1;           // 1 bit boolean
    u8 reserved : 3;           // Reserved for future use
  } packed;

  Vec4 offset;
  M4x4 model;

  // Compressed hit position using 16-bit integers (saves 8 bytes)
  // Range: -32768 to 32767 (sufficient for block coordinates)
  struct CompressedVec4 {
    s16 x, y, z, w;

    inline Vec4 toVec4() const {
      return Vec4((float)x, (float)y, (float)z, (float)w);
    }

    inline void fromVec4(const Vec4& v) {
      x = (s16)v.x;
      y = (s16)v.y;
      z = (s16)v.z;
      w = (s16)v.w;
    }
  } compressedHitPosition;

  // Keep frequently accessed data together
  float damage = 0;
  float distance = 0.0f;

  // Color can be compressed to 32-bit RGBA if needed
  Color baseColor;

  /**
   * Order: Top, Bottom, Left, Right, Back, Front
   * @param facesMapIndex 6 length array of texture index
   */
  virtual std::array<u8, 6> getFacesMap() = 0;
  virtual u8 isBreakable() = 0;
  virtual u8 isCollidable() = 0;
  virtual u8 hasTransparency() = 0;
  virtual u8 isCrossed() = 0;
  virtual Blocks getType() = 0;
  virtual float getHardness() = 0;

  // Accessors for packed data
  inline u16 getChunkId() const { return packed.chunkId; }
  inline void setChunkId(u16 id) { packed.chunkId = id; }

  inline u16 getLocalIndex() const { return packed.localIndex; }
  inline void setLocalIndex(u16 index) { packed.localIndex = index; }

  inline u16 getDrawDataIndex() const { return packed.drawDataIndex; }
  inline void setDrawDataIndex(u16 index) { packed.drawDataIndex = index; }

  inline u8 getDrawDataLength() const { return packed.drawDataLength; }
  inline void setDrawDataLength(u8 length) { packed.drawDataLength = length; }

  inline u8 getVisibleFaces() const { return packed.visibleFaces; }
  inline void setVisibleFaces(u8 faces) { packed.visibleFaces = faces; }

  inline u8 getVisibleFacesCount() const { return packed.visibleFacesCount; }
  inline void setVisibleFacesCount(u8 count) {
    packed.visibleFacesCount = count;
  }

  inline bool getIsTarget() const { return packed.isTarget; }
  inline void setIsTarget(bool target) { packed.isTarget = target ? 1 : 0; }

  // Hit position accessors
  inline Vec4 getHitPosition() const { return compressedHitPosition.toVec4(); }
  inline void setHitPosition(const Vec4& pos) {
    compressedHitPosition.fromVec4(pos);
  }

  // Note: Direct member access maintained for backward compatibility
  // These provide reference access to the packed members

  inline const bool isFrontFaceVisible() {
    return (getVisibleFaces() & FRONT_VISIBLE) == FRONT_VISIBLE;
  };

  inline const bool isBackFaceVisible() {
    return (getVisibleFaces() & BACK_VISIBLE) == BACK_VISIBLE;
  };

  inline const bool isLeftFaceVisible() {
    return (getVisibleFaces() & LEFT_VISIBLE) == LEFT_VISIBLE;
  };

  inline const bool isRightFaceVisible() {
    return (getVisibleFaces() & RIGHT_VISIBLE) == RIGHT_VISIBLE;
  };

  inline const bool isTopFaceVisible() {
    return (getVisibleFaces() & TOP_VISIBLE) == TOP_VISIBLE;
  };

  inline const bool isBottomFaceVisible() {
    return (getVisibleFaces() & BOTTOM_VISIBLE) == BOTTOM_VISIBLE;
  };

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
