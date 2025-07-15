#pragma once

#include "entities/Block.hpp"

// Stone bricks
class CrackedStoneBricksBlock : public Block {
 public:
  CrackedStoneBricksBlock() : Block() { collidable = isCollidable(); };
  virtual ~CrackedStoneBricksBlock() = default;
  Blocks getType() override { return Blocks::CRACKED_STONE_BRICKS_BLOCK; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {96, 96, 96, 96, 96, 96}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class StoneBrickBlock : public Block {
 public:
  StoneBrickBlock() : Block() { collidable = isCollidable(); };
  virtual ~StoneBrickBlock() = default;
  Blocks getType() override { return Blocks::STONE_BRICK_BLOCK; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {97, 97, 97, 97, 97, 97}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class MossyStoneBricksBlock : public Block {
 public:
  MossyStoneBricksBlock() : Block() { collidable = isCollidable(); };
  virtual ~MossyStoneBricksBlock() = default;
  Blocks getType() override { return Blocks::MOSSY_STONE_BRICKS_BLOCK; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {98, 98, 98, 98, 98, 98}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class ChiseledStoneBricksBlock : public Block {
 public:
  ChiseledStoneBricksBlock() : Block() { collidable = isCollidable(); };
  virtual ~ChiseledStoneBricksBlock() = default;
  Blocks getType() override { return Blocks::CHISELED_STONE_BRICKS_BLOCK; }
  float getHardness() override { return 1.5f; }
  std::array<u8, 6> getFacesMap() override { return {99, 99, 99, 99, 99, 99}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

// Wools
class YellowWool : public Block {
 public:
  YellowWool() : Block() { collidable = isCollidable(); };
  virtual ~YellowWool() = default;
  Blocks getType() override { return Blocks::YELLOW_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {102, 102, 102, 102, 102, 102};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class BlueWool : public Block {
 public:
  BlueWool() : Block() { collidable = isCollidable(); };
  virtual ~BlueWool() = default;
  Blocks getType() override { return Blocks::BLUE_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {103, 103, 103, 103, 103, 103};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class GreenWool : public Block {
 public:
  GreenWool() : Block() { collidable = isCollidable(); };
  virtual ~GreenWool() = default;
  Blocks getType() override { return Blocks::GREEN_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {104, 104, 104, 104, 104, 104};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class OrangeWool : public Block {
 public:
  OrangeWool() : Block() { collidable = isCollidable(); };
  virtual ~OrangeWool() = default;
  Blocks getType() override { return Blocks::ORANGE_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {105, 105, 105, 105, 105, 105};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class PurpleWool : public Block {
 public:
  PurpleWool() : Block() { collidable = isCollidable(); };
  virtual ~PurpleWool() = default;
  Blocks getType() override { return Blocks::PURPLE_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {106, 106, 106, 106, 106, 106};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class RedWool : public Block {
 public:
  RedWool() : Block() { collidable = isCollidable(); };
  virtual ~RedWool() = default;
  Blocks getType() override { return Blocks::RED_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {107, 107, 107, 107, 107, 107};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class WhiteWool : public Block {
 public:
  WhiteWool() : Block() { collidable = isCollidable(); };
  virtual ~WhiteWool() = default;
  Blocks getType() override { return Blocks::WHITE_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {108, 108, 108, 108, 108, 108};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class BlackWool : public Block {
 public:
  BlackWool() : Block() { collidable = isCollidable(); };
  virtual ~BlackWool() = default;
  Blocks getType() override { return Blocks::BLACK_WOOL; }
  float getHardness() override { return 1.8f; }
  std::array<u8, 6> getFacesMap() override {
    return {109, 109, 109, 109, 109, 109};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

// Logs
class OakLogBlock : public Block {
 public:
  OakLogBlock() : Block() { collidable = isCollidable(); };
  virtual ~OakLogBlock() = default;
  Blocks getType() override { return Blocks::OAK_LOG_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override { return {1, 1, 17, 17, 17, 17}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

class BirchLogBlock : public Block {
 public:
  BirchLogBlock() : Block() { collidable = isCollidable(); };
  virtual ~BirchLogBlock() = default;
  Blocks getType() override { return Blocks::BIRCH_LOG_BLOCK; }
  float getHardness() override { return 2.0f; }
  std::array<u8, 6> getFacesMap() override { return {6, 6, 22, 22, 22, 22}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};

// Leaves
class BirchLeavesBlock : public Block {
 public:
  BirchLeavesBlock() : Block() { collidable = isCollidable(); };
  virtual ~BirchLeavesBlock() = default;
  Blocks getType() override { return Blocks::BIRCH_LEAVES_BLOCK; }
  float getHardness() override { return 0.2f; }
  std::array<u8, 6> getFacesMap() override {
    return {162, 162, 162, 162, 162, 162};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

class OakLeavesBlock : public Block {
 public:
  OakLeavesBlock() : Block() { collidable = isCollidable(); };
  virtual ~OakLeavesBlock() = default;
  Blocks getType() override { return Blocks::OAK_LEAVES_BLOCK; }
  float getHardness() override { return 0.2f; }
  std::array<u8, 6> getFacesMap() override {
    return {163, 163, 163, 163, 163, 163};
  }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};

// Items
class Torch : public Block {
 public:
  Torch() : Block() { collidable = isCollidable(); };
  virtual ~Torch() = default;
  Blocks getType() override { return Blocks::TORCH; }
  float getHardness() override { return 0.001f; }
  std::array<u8, 6> getFacesMap() override { return {1, 1, 1, 1, 1, 1}; }
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }
  u8 hasTransparency() override { return true; }
  u8 isCrossed() override { return false; }
};
