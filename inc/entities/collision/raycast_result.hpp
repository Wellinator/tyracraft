#pragma once

#include "constants.hpp"
#include <tyra>

using Tyra::Vec4;

struct RayCastResult {
  bool hit;
  //   Collider* collider;
  Vec4 position;
  Vec4 normal;
};