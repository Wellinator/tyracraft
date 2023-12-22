#pragma once

#include "constants.hpp"
#include <cstdint>
#include <tyra>

using Tyra::BBox;
using Tyra::Vec4;

class Entity {
 public:
  Entity(const EntityType type) : entity_type(type){};

  Entity(const EntityType type, const Vec4 position, BBox* bbox)
      : entity_type(type) {
    this->position.set(position);
    this->bbox = bbox;
    this->bbox->getMinMax(&minCorner, &maxCorner);
  };

  Entity(const EntityType type, const Vec4 position, BBox* bbox, const Vec4 min,
         const Vec4 max)
      : entity_type(type) {
    this->position.set(position);
    this->minCorner.set(min);
    this->maxCorner.set(max);
    this->bbox = bbox;
  };

  virtual ~Entity(){};

  // Index at aabb tree;
  int32_t tree_index;

  const EntityType entity_type;

  Vec4 position, velocity;
  Vec4 minCorner, maxCorner;

  BBox* bbox = nullptr;
};
