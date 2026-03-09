#pragma once

#include "managers/particle/particle.hpp"
#include "entities/level.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include <tyra>

using Tyra::Vec4;
using Tyra::M4x4;


/**
 * Intermediate particle class for particles that collide with the world grid.
 * Uses per-axis direct grid lookups — no heap allocation, O(1) per axis.
 * On collision, velocity on the colliding axis is zeroed (slide & settle).
 */
class CollidableParticle : public Particle {
 public:
  CollidableParticle(const ParticleType& _type);
  virtual ~CollidableParticle() {};

  Vec4 _prevPosition = Vec4(0.0F);
  Vec4 _targetPosition = Vec4(0.0F);

 protected:
  /**
   * Resolve collision between the current position and the next position
   * using axis-separated direct grid lookups. Updates _targetPosition and
   * zeroes _velocity on the colliding axis (slide & settle behaviour).
   */
  void resolveCollision(Vec4& nextPosition);

 private:
  /**
   * Returns true if the world-space position contains a solid (collidable)
   * block. Out-of-bounds positions are treated as solid.
   */
  u8 isBlockCollidableAtWorldPos(Level* level, const Vec4& worldPos);
};
