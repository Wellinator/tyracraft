#pragma once

#include <tyra>

using Tyra::Vec4;

struct CustomMeshOptions {
  float scale = 1.0f;
  Vec4 rotation = Vec4(0, 0, 0);
  Vec4 translation = Vec4(0, 0, 0);
};
