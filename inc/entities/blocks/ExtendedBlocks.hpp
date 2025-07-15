#pragma once

#include "entities/Block.hpp"

// Basic blocks
class SandBlock : public Block {
 public:
  SandBlock() : Block() { collidable = isCollidable(); };
  virtual ~SandBlock() = default;
  Blocks getType() override { return Blocks::SAND_BLOCK; }
  float getHardness() override { return 0.5f; }
  std::array<u8, 6> getFacesMap() override {
    return {114, 114, 114, 114, 114, 114};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class GlassBlock : public Block {
 public:
  GlassBlock() : Block() { collidable = isCollidable(); };
  virtual ~GlassBlock() = default;
  Blocks getType() override { return Blocks::GLASS_BLOCK; }
  float getHardness() override { return 0.3f; }
  std::array<u8, 6> getFacesMap() override {
    return {144, 144, 144, 144, 144, 144};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class BricksBlock : public Block {
 public:
  BricksBlock() : Block() { collidable = isCollidable(); };
  virtual ~BricksBlock() = default;
  Blocks getType() override { return Blocks::BRICKS_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {100, 100, 100, 100, 100, 100};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class GravelBlock : public Block {
 public:
  GravelBlock() : Block() { collidable = isCollidable(); };
  virtual ~GravelBlock() = default;
  Blocks getType() override { return Blocks::GRAVEL_BLOCK; }
  float getHardness() override { return 0.6f; }
  std::array<u8, 6> getFacesMap() override {
    return {101, 101, 101, 101, 101, 101};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

// Face oriented blocks
class PumpkinBlock : public Block {
 public:
  PumpkinBlock() : Block() { collidable = isCollidable(); };
  virtual ~PumpkinBlock() = default;
  Blocks getType() override { return Blocks::PUMPKIN_BLOCK; }
  float getHardness() override { return 1.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {209, 209, 208, 208, 208, 211};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

// Wood Planks
class OakPlanksBlock : public Block {
 public:
  OakPlanksBlock() : Block() { collidable = isCollidable(); };
  virtual ~OakPlanksBlock() = default;
  Blocks getType() override { return Blocks::OAK_PLANKS_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {176, 176, 176, 176, 176, 176};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class SprucePlanksBlock : public Block {
 public:
  SprucePlanksBlock() : Block() { collidable = isCollidable(); };
  virtual ~SprucePlanksBlock() = default;
  Blocks getType() override { return Blocks::SPRUCE_PLANKS_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {177, 177, 177, 177, 177, 177};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class AcaciaPlanksBlock : public Block {
 public:
  AcaciaPlanksBlock() : Block() { collidable = isCollidable(); };
  virtual ~AcaciaPlanksBlock() = default;
  Blocks getType() override { return Blocks::ACACIA_PLANKS_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {178, 178, 178, 178, 178, 178};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class BirchPlanksBlock : public Block {
 public:
  BirchPlanksBlock() : Block() { collidable = isCollidable(); };
  virtual ~BirchPlanksBlock() = default;
  Blocks getType() override { return Blocks::BIRCH_PLANKS_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {179, 179, 179, 179, 179, 179};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

// Light Emissors
class GlowstoneBlock : public Block {
 public:
  GlowstoneBlock() : Block() { collidable = isCollidable(); };
  virtual ~GlowstoneBlock() = default;
  Blocks getType() override { return Blocks::GLOWSTONE_BLOCK; }
  float getHardness() override { return 0.3f; }
  std::array<u8, 6> getFacesMap() override {
    return {212, 212, 212, 212, 212, 212};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class JackOLanternBlock : public Block {
 public:
  JackOLanternBlock() : Block() { collidable = isCollidable(); };
  virtual ~JackOLanternBlock() = default;
  Blocks getType() override { return Blocks::JACK_O_LANTERN_BLOCK; }
  float getHardness() override { return 1.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {209, 209, 208, 208, 208, 210};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class LavaBlock : public Block {
 public:
  LavaBlock() : Block() { collidable = isCollidable(); };
  virtual ~LavaBlock() = default;
  Blocks getType() override { return Blocks::LAVA_BLOCK; }
  float getHardness() override { return 0.0f; }
  std::array<u8, 6> getFacesMap() override { return {80, 80, 80, 80, 80, 80}; }
  u8 isBreakable() override { return false; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};
