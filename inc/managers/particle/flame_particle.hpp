#pragma once

#include "entities/Block.hpp"
#include "particle.hpp"

class FlameParticle : public Particle {
 public:
  const float START_SIZE = 3.0F;

  FlameParticle(Vec4* offset);
  void fixedUpdate(const float fixedDeltaTime);
  void update(const float deltaTime, const Vec4* camPos);
};