#pragma once

#include "entities/Block.hpp"

// Flowers
class GrassPlant : public Block {
 public:
  GrassPlant() : Block() { collidable = isCollidable(); };
  virtual ~GrassPlant() = default;

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
