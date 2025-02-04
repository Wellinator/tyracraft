#pragma once

#include "entities/Block.hpp"
#include "particle.hpp"

class SmokeParticle : public Particle {
 public:
  // Number of smoke particle stages
  const u8 MAX_UV_INDEX = 7;
  s8 stageIndex = -1;

  const float size = 3.0F;
  float _elapsedFrameTime = 0;

  SmokeParticle(Block* pBlock);
  ~SmokeParticle();

  void fixedUpdate(const float fixedDeltaTime);
  void update(const float deltaTime, const Vec4* camPos);

  u8 getStage();
  void updateUV(const u8 _stageIndex);

 private:
  Vec4 _prevPosition = Vec4(0.0F), _targetPosition = Vec4(0.0F);
};