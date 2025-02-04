#pragma once

#include "constants.hpp"
#include <tyra>

using Tyra::Color;
using Tyra::M4x4;
using Tyra::Vec4;

class Particle {
 public:
  Particle(const PaticleType& _type) : type(_type){};
  virtual ~Particle(){};

  u8 isAllive() { return !expired; }

  // Vituals
  virtual void fixedUpdate(const float fixedDeltaTime) = 0;
  virtual void update(const float deltaTime, const Vec4* camPos) = 0;
  void virtual renew() {
    _elapsedTime = 0;
    expired = false;
  }

  const PaticleType type;
  u32 id = 0;
  u8 billboarded = true;
  u8 expired = false;
  u8 collidable = false;
  Color color;

  static const u8 DRAW_DATA_COUNT = 6;
  static const Vec4 rawData[DRAW_DATA_COUNT];

 public:
  float _elapsedTime = 0;
  float _lifeTime = 0;
  Vec4 _velocity = Vec4(0.0F);
  Vec4 _position = Vec4(0.0F);
  Vec4 _direction = Vec4(0.0F);
  Vec4 uv[6] = {};
  Vec4 vertex[6] = {};
};
