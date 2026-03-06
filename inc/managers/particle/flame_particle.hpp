#pragma once

#include "entities/Block.hpp"
#include "particle.hpp"

class FlameParticle : public Particle {
 public:
  const float START_SIZE = 3.0F;

  static const Vec4 flameUV[6];  // Shared static UV — same for all flames

  FlameParticle(Vec4* offset);
  void fixedUpdate(const float fixedDeltaTime);
  void update(const float deltaTime, const M4x4* billboard) override;
};