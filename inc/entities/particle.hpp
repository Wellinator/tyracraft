#pragma once

#include <vector>
#include "tyra"

using Tyra::M4x4;
using Tyra::Texture;
using Tyra::Vec4;

class Particle {
 public:
  Particle();
  ~Particle();

  u8 expired = false;
  u8 col;
  u8 row;

  M4x4 model, translation, rotation, scale;

  float _elapsedTime = 0;
  float _lifeTime;
  Vec4 _velocity;
  Vec4 _position;
  Vec4 _direction;
};
