#pragma once

#include "entities/Block.hpp"

class StoneBlock : public Block {
 public:
  StoneBlock() : Block() { collidable = isCollidable(); };
  virtual ~StoneBlock() = default;

  IMPLEMENT_BLOCK_CLONE(StoneBlock)

  Blocks getType() override { return Blocks::STONE_BLOCK; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override {
    return {115, 115, 115, 115, 115, 115};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class GrassBlock : public Block {
 public:
  GrassBlock() : Block() {
    collidable = isCollidable();
  };
  // virtual ~GrassBlock() = default;

  IMPLEMENT_BLOCK_CLONE(GrassBlock)

  Blocks getType() override { return Blocks::GRASS_BLOCK; }
  float getHardness() override { return 0.6f; }
  std::array<u8, 6> getFacesMap() override {
    return {0, 113, 16, 16, 16, 16};  // Top, Bottom, Left, Right, Back, Front
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class DirtyBlock : public Block {
 public:
  DirtyBlock() : Block() { collidable = isCollidable(); };
  virtual ~DirtyBlock() = default;

  IMPLEMENT_BLOCK_CLONE(DirtyBlock)

  Blocks getType() override { return Blocks::DIRTY_BLOCK; }
  float getHardness() override { return 0.5f; }
  std::array<u8, 6> getFacesMap() override {
    return {113, 113, 113, 113, 113, 113};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class WaterBlock : public Block {
 public:
  WaterBlock() : Block() { collidable = isCollidable(); };
  virtual ~WaterBlock() = default;

  IMPLEMENT_BLOCK_CLONE(WaterBlock)

  Blocks getType() override { return Blocks::WATER_BLOCK; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {116, 116, 116, 116, 116, 116};
  }
  u8 isBreakable() override { return false; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class BedrockBlock : public Block {
 public:
  BedrockBlock() : Block() { collidable = isCollidable(); };
  virtual ~BedrockBlock() = default;

  IMPLEMENT_BLOCK_CLONE(BedrockBlock)

  Blocks getType() override { return Blocks::BEDROCK_BLOCK; }
  float getHardness() override { return -1.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {112, 112, 112, 112, 112, 112};
  }
  u8 isBreakable() override { return false; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};
