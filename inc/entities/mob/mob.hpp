#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/entity.hpp"
#include "entities/level.hpp"
#include "entities/mob/AI/path_result.hpp"
#include "entities/mob/AI/mob_state.hpp"
#include "3libs/bvh/bvh.h"
#include <memory>

class Mob : public Entity {
 public:
  Mob(Level* level);
  virtual ~Mob();

  virtual void fixedUpdate(const float& fixedDeltaTime);
  virtual void update(const float& deltaTime);
  virtual void render() = 0;

  virtual void jump();
  virtual void onMoved() = 0;
  virtual void onStopMoving() = 0;
  virtual Vec4 getNextXZPosition(const float& fixedDeltaTime,
                                 const Vec4& target) = 0;
  virtual bool updateXZPosition(const float& fixedDeltaTime,
                                const Vec4& nextPosition,
                                u8 isColliding = 0) = 0;

  virtual inline Vec4* getPosition() { return &position; };

  virtual void setPosition(const Vec4& pos) {
    position.set(pos);

    // mesh.get()->getPosition()->set(position);
    //  TODO: move mesh position to render matrix

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
  Tyra::Mesh** framesArray = nullptr;
  const int framesCount = 0;

  Vec4 spawnPosition;
  Vec4 lookAt;

  u8 fullProcessing = true;
  u8 shouldUnspawn = false;
  u8 collidable = true;

  // AI
  MobState* currentState;
  std::unique_ptr<PathResult> currentPath;
  bool advancePath(const float& fixedDeltaTime, bool faceRoute = true);

 protected:
  // Phisycs values
  const Vec4 lift = Vec4(0.0f, 120.0F, 0.0f);
};
