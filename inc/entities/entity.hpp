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
  Entity(Level* level, const EntityType type) : entity_type(type) {
    pLevel = level;
  };

  virtual ~Entity(){};

  virtual void tick(){};

  Level* pLevel;

  // Index at aabb tree;
  int32_t tree_index;

  const EntityType entity_type;

  Vec4 position = Vec4(0, 0, 0), _prevPosition = Vec4(0.0F),
       _targetPosition = Vec4(0.0F);

  Vec4 velocity = Vec4(0, 0, 0);
  Vec4 minCorner, maxCorner;
  TerrainHeightModel terrainHeight;
  Entity* underEntity = nullptr;
  Entity* overEntity = nullptr;

  u8 collidable = false;

  // Entity dynamic states
  bool isOnGround, isMoving, _isOnWater, _isUnderWater;

  virtual const float getHeight() { return 0.0f; }

  // Check for details https://minecraft.fandom.com/wiki/Hitbox
  //  protected:
  BBox* bbox = nullptr;

 public:
  virtual const BBox getHitBox() {
    M4x4 translation = M4x4::Identity;
    translation.translate(_targetPosition);
    return bbox->getTransformed(translation);
  };

  virtual const BBox getHitBox(const M4x4& model) {
    return bbox->getTransformed(model);
  };

  virtual void resolveOutOfWorldBoundaries() {
    TYRA_TRAP("resolveOutOfWorldBoundaries not implemented!");
  };

  virtual void updateTerrainHeightAtEntityPosition() {
    BBox entityBB = getHitBox();
    Vec4 minEntity, maxEntity;
    entityBB.getMinMax(&minEntity, &maxEntity);

    terrainHeight.reset();
    underEntity = nullptr;
    overEntity = nullptr;

    // Prepate the raycast
    const Vec4 offset = Vec4(0, 40, 0);
    Vec4 segmentStart = maxEntity + offset;
    Vec4 segmentEnd = minEntity - offset;

    std::vector<int32_t> ni;
    g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

    for (u16 i = 0; i < ni.size(); i++) {
      Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);
      if (!entity->collidable) continue;

      // is under or above entity
      if (minEntity.x <= entity->maxCorner.x &&
          maxEntity.x >= entity->minCorner.x &&
          minEntity.z <= entity->maxCorner.z &&
          maxEntity.z >= entity->minCorner.z) {
        const float minHeight = entity->maxCorner.y;
        if (minEntity.y >= minHeight && minHeight > terrainHeight.minHeight) {
          terrainHeight.minHeight = minHeight;
          underEntity = entity;
        }

        const float maxHeight = entity->minCorner.y;
        if (maxEntity.y < maxHeight && maxHeight < terrainHeight.maxHeight) {
          terrainHeight.maxHeight = maxHeight;
          overEntity = entity;
        }
      }
    }
  }

  /** Update entity position by gravity and update index of current block */
  virtual void updateYPosition(const float nextYPos) {
    float resultY = nextYPos;
    const float worldMinHeight = OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE;
    const float worldMaxHeight = OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE;
    const float enityHeight = getHeight();

    if (nextYPos + enityHeight > worldMaxHeight || nextYPos < worldMinHeight) {
      return resolveOutOfWorldBoundaries();
    }

    const float heightLimit = terrainHeight.maxHeight - enityHeight;

    if (resultY < terrainHeight.minHeight) {
      resultY = terrainHeight.minHeight;
      velocity.y = 0.0f;
      isOnGround = true;
    } else if (resultY >= heightLimit) {
      resultY = heightLimit;
      velocity.y = -velocity.y;
      isOnGround = false;
    }

    // Finally updates gravity after checks
    _targetPosition.y = resultY;
  }
};
