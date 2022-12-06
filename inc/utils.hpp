/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#pragma once

#include <tamtypes.h>
#include <renderer/3d/mesh/mesh.hpp>
#include <math/vec4.hpp>
#include <math/math.hpp>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

using Tyra::BBox;
using Tyra::CoreBBoxFrustum;
using Tyra::Mesh;
using Tyra::Plane;
using Tyra::Vec4;

// enum eDirection { RIGHT, LEFT, UP, DOWN, FRONT, BACK };

struct Rect {
  Rect(const Vec4& _min, const Vec4& _max, const Vec4& _velocity) {
    min.set(_min);
    max.set(_max);
    velocity.set(_velocity);
  }

  Rect(const Vec4& _min, const Vec4& _max) {
    min.set(_min);
    max.set(_max);
    velocity = Vec4();
  }

  Vec4 min;
  Vec4 max;
  Vec4 velocity;
};

class Utils {
 public:
  static float degreesToRadian(float degress);
  static float expoEaseInOut(float t, float b, float c, float d);
  // static void getMinMax(const Mesh &t_mesh, Vec4 &t_min, Vec4 &t_max);
  static float clamp(const float value, float min, float max);
  static float FOG_LINEAR(const float& d, const float& start, const float& end,
                          const float& offset = 0.0F);
  static float FOG_EXP(float d, float density);
  static float FOG_EXP2(float d, float density);
  static float FOG_EXP_GRAD(float d, float density, float gradient);
  static float Raycast(Vec4* origin, Vec4* dir, Vec4* min, Vec4* max);
  static Vec4 GetNormalFromHitPosition(const Vec4& intersection,
                                       const Vec4& min, const Vec4& max);
  static void GetMinkowskiSum(const Vec4& AMin, const Vec4& AMax,
                              const Vec4& BMin, const Vec4& BMax,
                              Vec4* resultMin, Vec4* resultMax);

  static CoreBBoxFrustum FrustumAABBIntersect(const Plane* frustumPlanes,
                                              const BBox& AABB);
  static CoreBBoxFrustum FrustumAABBIntersect(const Plane* frustumPlanes,
                                              const Vec4& mins,
                                              const Vec4& maxs);

  static float fastPow(float a, float b);

  /* Schraudolph's published algorithm with John's constants */
  /* 1065353216 - 486411 = 1064866805 */
  static float expf_fast(float a);
};
