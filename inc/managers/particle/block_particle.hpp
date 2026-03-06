#pragma once

#include "entities/Block.hpp"
#include "managers/particle/particle.hpp"
#include "managers/collision_manager.hpp"
#include "utils.hpp"
#include "timer.hpp"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;

class BlockParticle : public Particle {
 public:
  const float _scale = Tyra::Math::randomf(0.5F, 1.0F);
  Vec4 _prevPosition = Vec4(0.0F), _targetPosition = Vec4(0.0F);

  explicit BlockParticle(Block* pBlock);
  void fixedUpdate(const float fixedDeltaTime);
  void update(const float deltaTime, const M4x4* billboard) override;
};