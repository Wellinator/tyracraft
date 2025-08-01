#pragma once

#include "entities/Block.hpp"

// Slabs
class StoneSlab : public Block {
 public:
  StoneSlab() : Block() { collidable = isCollidable(); };
  virtual ~StoneSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(StoneSlab)
  
  Blocks getType() override { return Blocks::STONE_SLAB; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override {
    return {115, 115, 115, 115, 115, 115};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class BricksSlab : public Block {
 public:
  BricksSlab() : Block() { collidable = isCollidable(); };
  virtual ~BricksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(BricksSlab)
  
  Blocks getType() override { return Blocks::BRICKS_SLAB; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {100, 100, 100, 100, 100, 100};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class OakPlanksSlab : public Block {
 public:
  OakPlanksSlab() : Block() { collidable = isCollidable(); };
  virtual ~OakPlanksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(OakPlanksSlab)
  
  Blocks getType() override { return Blocks::OAK_PLANKS_SLAB; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {176, 176, 176, 176, 176, 176};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class SprucePlanksSlab : public Block {
 public:
  SprucePlanksSlab() : Block() { collidable = isCollidable(); };
  virtual ~SprucePlanksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(SprucePlanksSlab)
  
  Blocks getType() override { return Blocks::SPRUCE_PLANKS_SLAB; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {177, 177, 177, 177, 177, 177};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class AcaciaPlanksSlab : public Block {
 public:
  AcaciaPlanksSlab() : Block() { collidable = isCollidable(); };
  virtual ~AcaciaPlanksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(AcaciaPlanksSlab)
  
  Blocks getType() override { return Blocks::ACACIA_PLANKS_SLAB; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {178, 178, 178, 178, 178, 178};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class BirchPlanksSlab : public Block {
 public:
  BirchPlanksSlab() : Block() { collidable = isCollidable(); };
  virtual ~BirchPlanksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(BirchPlanksSlab)
  
  Blocks getType() override { return Blocks::BIRCH_PLANKS_SLAB; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {179, 179, 179, 179, 179, 179};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class CrackedStoneBricksSlab : public Block {
 public:
  CrackedStoneBricksSlab() : Block() { collidable = isCollidable(); };
  virtual ~CrackedStoneBricksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(CrackedStoneBricksSlab)
  
  Blocks getType() override { return Blocks::CRACKED_STONE_BRICKS_SLAB; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {96, 96, 96, 96, 96, 96}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class StoneBrickSlab : public Block {
 public:
  StoneBrickSlab() : Block() { collidable = isCollidable(); };
  virtual ~StoneBrickSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(StoneBrickSlab)
  
  Blocks getType() override { return Blocks::STONE_BRICK_SLAB; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {97, 97, 97, 97, 97, 97}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class MossyStoneBricksSlab : public Block {
 public:
  MossyStoneBricksSlab() : Block() { collidable = isCollidable(); };
  virtual ~MossyStoneBricksSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(MossyStoneBricksSlab)
  
  Blocks getType() override { return Blocks::MOSSY_STONE_BRICKS_SLAB; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {98, 98, 98, 98, 98, 98}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};
