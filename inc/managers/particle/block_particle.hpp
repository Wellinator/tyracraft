#pragma once

#include "entities/Block.hpp"
#include "managers/particle/collidable_particle.hpp"
#include "utils.hpp"
#include "timer.hpp"

class BlockParticle : public CollidableParticle {
 public:
  const float _scale = Tyra::Math::randomf(0.5F, 1.0F);

  explicit BlockParticle(Block* pBlock);
  void fixedUpdate(const float fixedDeltaTime);
  void update(const float deltaTime, const M4x4* billboard) override;
};