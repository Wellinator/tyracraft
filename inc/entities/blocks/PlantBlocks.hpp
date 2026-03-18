#pragma once

#include "entities/Block.hpp"

// Flowers
class GrassPlant : public Block {
 public:
  GrassPlant() : Block() { collidable = isCollidable(); };
  virtual ~GrassPlant() = default;

  IMPLEMENT_BLOCK_CLONE(GrassPlant)

  Blocks getType() override { return Blocks::GRASS; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {169, 169, 169, 169, 169, 169};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return true; }
};

class PoppyFlower : public Block {
 public:
  PoppyFlower() : Block() { collidable = isCollidable(); };
  virtual ~PoppyFlower() = default;

  IMPLEMENT_BLOCK_CLONE(PoppyFlower)

  Blocks getType() override { return Blocks::POPPY_FLOWER; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {167, 167, 167, 167, 167, 167};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return true; }
};

class DandelionFlower : public Block {
 public:
  DandelionFlower() : Block() { collidable = isCollidable(); };
  virtual ~DandelionFlower() = default;

  IMPLEMENT_BLOCK_CLONE(DandelionFlower)

  Blocks getType() override { return Blocks::DANDELION_FLOWER; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {168, 168, 168, 168, 168, 168};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return true; }
};

class DeadBushBlock : public Block {
 public:
  DeadBushBlock() : Block() { collidable = isCollidable(); };
  virtual ~DeadBushBlock() = default;

  IMPLEMENT_BLOCK_CLONE(DeadBushBlock)

  Blocks getType() override { return Blocks::DEAD_BUSH; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {170, 170, 170, 170, 170, 170};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return true; }
};

class ReedsPlant : public Block {
 public:
  ReedsPlant() : Block() { collidable = isCollidable(); };
  virtual ~ReedsPlant() = default;

  IMPLEMENT_BLOCK_CLONE(ReedsPlant)

  Blocks getType() override { return Blocks::REEDS_BLOCK; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {171, 171, 171, 171, 171, 171};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return true; }
};

class TallGrassPlant : public Block {
 public:
  TallGrassPlant() : Block() { collidable = isCollidable(); };
  virtual ~TallGrassPlant() = default;

  IMPLEMENT_BLOCK_CLONE(TallGrassPlant)

  Blocks getType() override { return Blocks::TALL_GRASS_BLOCK; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {165, 165, 165, 165, 165, 165};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return true; }
};
