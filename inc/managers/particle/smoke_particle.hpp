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

  SmokeParticle(Vec4* offset);
  ~SmokeParticle();

  void fixedUpdate(const float fixedDeltaTime);
  void update(const float deltaTime, const M4x4* billboard) override;

  u8 getStage();
  void updateUV(const u8 _stageIndex);

  // UV look-up table: 8 animation stages × 6 vertices (built once at init)
  static Vec4** uvLUT;
  static void initUVLUT();
  static void destroyUVLUT();

 private:
  Vec4 _prevPosition = Vec4(0.0F), _targetPosition = Vec4(0.0F);
};