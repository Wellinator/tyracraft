#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/entity.hpp"
#include "entities/level.hpp"
#include "3libs/bvh/bvh.h"
#include <memory>

using Tyra::DynamicMesh;

class Mob : public Entity {
 public:
  Mob(Level* level) : Entity(level, EntityType::Mob) {
    t_near_entities = new std::vector<bvh::index_t>();
  };

  virtual ~Mob() {
    t_near_entities->clear();
    t_near_entities->shrink_to_fit();
    delete t_near_entities;
  };

  // TODO: Replace the movementDir param by pathFinder
  virtual void fixedUpdate(const float& fixedDeltaTime,
                           const Vec4& movementDir) = 0;
  virtual void update(const float& deltaTime) = 0;
  virtual void render() = 0;

  virtual inline Vec4* getPosition() { return &position; };
  virtual void setPosition(const Vec4& pos) {
    position.set(pos);
    mesh->getPosition()->set(position);

    _prevPosition.set(position);
    _targetPosition.set(position);
  };

  /** Mob category */
  virtual MobCategory getCategory() {
    TYRA_TRAP("getCategory() not implemented!");
    return MobCategory::Invalid;
  };

  /** Mob type */
  virtual MobType getType() {
    TYRA_TRAP("getType() not implemented!");
    return MobType::Invalid;
  };

  /** Mod id */
  const uint32_t id = rand() % 999999;

  /** Mob mesh data */
  DynamicMesh* mesh;

  Vec4 spawnPosition;

  // TODO: move to mob AI
  /** Temp last moviment dir */
  Vec4 moviemntDirection;

  u8 fullProcessing = true;
  u8 shouldUnspawn = false;
  u8 collidable = true;
  std::vector<bvh::index_t>* t_near_entities = nullptr;
};
