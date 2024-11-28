#pragma once

#include "constants.hpp"
#include "entities/level.hpp"
#include <cstdint>
#include <tyra>

using Tyra::BBox;
using Tyra::Vec4;

class Entity {
 public:
  Entity(Level* level, const EntityType type) : entity_type(type) {
    pLevel = level;
  };

  // Entity(Level* level, const EntityType type, const Vec4 position, BBox*
  // bbox)
  //     : entity_type(type) {
  //   pLevel = level;
  //   this->position.set(position);
  //   this->bbox = bbox;
  //   this->bbox->getMinMax(&minCorner, &maxCorner);
  // };

  // Entity(Level* level, const EntityType type, const Vec4 position, BBox*
  // bbox,
  //        const Vec4 min, const Vec4 max)
  //     : entity_type(type) {
  //   pLevel = level;
  //   this->position.set(position);
  //   this->minCorner.set(min);
  //   this->maxCorner.set(max);
  //   this->bbox = bbox;
  // };

  Level* pLevel;

  virtual ~Entity(){};

  // Index at aabb tree;
  int32_t tree_index;

  const EntityType entity_type;

  Vec4 position = Vec4(0, 0, 0);
  Vec4 velocity = Vec4(0, 0, 0);
  Vec4 minCorner, maxCorner;

  // Check for details https://minecraft.fandom.com/wiki/Hitbox
  BBox* bbox = nullptr;

  u8 collidable = false;
};
