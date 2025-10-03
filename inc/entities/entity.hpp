#pragma once

#include "constants.hpp"
#include "entities/level.hpp"
#include "models/terrain_height_model.hpp"
#include "managers/collision_manager.hpp"
#include <cstdint>
#include <tyra>

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::Vec4;

class Entity {
 public:
  Entity(Level* level, const EntityType type);
  virtual ~Entity();
  virtual void tick();

  Level* pLevel;

  // Index at aabb tree;
  int32_t tree_index;

  const EntityType entity_type;

  Vec4 rotation = Vec4(0, 0, 0);
  Vec4 position = Vec4(0, 0, 0), _prevPosition = Vec4(0, 0, 0),
       _targetPosition = Vec4(0, 0, 0);

  Vec4 velocity = Vec4(0, 0, 0);
  Vec4 minCorner, maxCorner;
  TerrainHeightModel terrainHeight;

  u8 collidable = false;

  // Entity states
  bool isOnGround, isMoving, _isOnWater, _isUnderWater;

  virtual const float getHeight();

  // Check for details https://minecraft.fandom.com/wiki/Hitbox
  BBox* bbox = nullptr;

 public:
  virtual const BBox getHitBox();
  virtual const BBox getHitBox(const M4x4& model);
  virtual void resolveOutOfWorldBoundaries();
  virtual void updateTerrainHeightAtEntityPosition();
  /** Update entity position by gravity and update index of current block */
  virtual void updateYPosition(const float nextYPos);

  virtual const Vec4 getHitBoxSize() {
    TYRA_TRAP("getHitBoxSize() not implemented!");
    return Vec4(0, 0, 0);
  };

 protected:
  virtual void loadStaticBBox() {
    TYRA_TRAP("loadStaticBBox() not implemented!");
  };
};
