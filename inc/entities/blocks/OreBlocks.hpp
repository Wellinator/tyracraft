#pragma once

#include "entities/Block.hpp"

// Ores and Minerals
class GoldOreBlock : public Block {
 public:
  GoldOreBlock() : Block() { collidable = isCollidable(); };
  virtual ~GoldOreBlock() = default;

  Blocks getType() override { return Blocks::GOLD_ORE_BLOCK; }
  float getHardness() override { return 3.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {128, 128, 128, 128, 128, 128};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class RedstoneOreBlock : public Block {
 public:
  RedstoneOreBlock() : Block() { collidable = isCollidable(); };
  virtual ~RedstoneOreBlock() = default;

  Blocks getType() override { return Blocks::REDSTONE_ORE_BLOCK; }
  float getHardness() override { return 3.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {129, 129, 129, 129, 129, 129};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class IronOreBlock : public Block {
 public:
  IronOreBlock() : Block() { collidable = isCollidable(); };
  virtual ~IronOreBlock() = default;

  Blocks getType() override { return Blocks::IRON_ORE_BLOCK; }
  float getHardness() override { return 3.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {130, 130, 130, 130, 130, 130};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class EmeraldOreBlock : public Block {
 public:
  EmeraldOreBlock() : Block() { collidable = isCollidable(); };
  virtual ~EmeraldOreBlock() = default;

  Blocks getType() override { return Blocks::EMERALD_ORE_BLOCK; }
  float getHardness() override { return 3.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {131, 131, 131, 131, 131, 131};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class DiamondOreBlock : public Block {
 public:
  DiamondOreBlock() : Block() { collidable = isCollidable(); };
  virtual ~DiamondOreBlock() = default;

  Blocks getType() override { return Blocks::DIAMOND_ORE_BLOCK; }
  float getHardness() override { return 3.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {132, 132, 132, 132, 132, 132};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class CoalOreBlock : public Block {
 public:
  CoalOreBlock() : Block() { collidable = isCollidable(); };
  virtual ~CoalOreBlock() = default;

  Blocks getType() override { return Blocks::COAL_ORE_BLOCK; }
  float getHardness() override { return 3.0f; }
  std::array<u8, 6> getFacesMap() override {
    return {133, 133, 133, 133, 133, 133};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};
