#pragma once

#include "constants.hpp"
#include <tyra>

using Tyra::Color;
using Tyra::M4x4;
using Tyra::Vec4;

class Particle {
 public:
  Particle(const ParticleType& _type) : type(_type) {};
  virtual ~Particle() {};

  u8 isAllive() { return !expired; }

  // Vituals
  virtual void fixedUpdate(const float fixedDeltaTime) = 0;
  // billboard: pre-computed rotation matrix (lookAt inverse) from the manager.
  // All particles share the same rotation; each applies its own position/scale.
  virtual void update(const float deltaTime, const M4x4* billboard) = 0;

  // tick: game-logic update driven by the TickManager (20 TPS, same as
  // Minecraft). Lifetime counters, state changes, and anything that should
  // freeze when ticks are paused belong here. Default is a no-op so that
  // particles that don't need tick logic don't pay for a vtable call.
  virtual void tick() {
    if (_lifeTime == 0) {
      expired = true;
      return;
    }

    --_lifeTime;
  }

  // renew: reset particle state for reuse (override in derived classes if
  // needed)
  virtual void renew() { expired = false; }

  const ParticleType type;
  u32 id = 0;
  u8 billboarded = true;
  u8 expired = false;
  u8 collidable = false;
  Color color;

  static const u8 DRAW_DATA_COUNT = 6;
  static const Vec4 rawData[DRAW_DATA_COUNT];

 public:
  // Common properties for all particle types

  // Lifetime in ticks
  int _lifeTime = 0;

  Vec4 _velocity = Vec4(0.0F);
  Vec4 _position = Vec4(0.0F);
  Vec4 _direction = Vec4(0.0F);
  Vec4 uv[6] = {};
  Vec4 vertex[6] = {};
};
