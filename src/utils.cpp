/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "./utils.hpp"
#include <fastmath.h>
#include <physics/ray.hpp>
#include <renderer/3d/bbox/bbox.hpp>

using Tyra::BBox;
using Tyra::Math;
using Tyra::Mesh;
using Tyra::Ray;
using Tyra::Vec4;

/** Degrees to Radian conversion
 * @param degress Value in degress
 * @returns Value in radians
 */
float Utils::degreesToRadian(float degress) {
  return degress * (Math::PI / 180);
}

/** Exponential ease-in-out animation
 * @param t Time
 * @param b Start value
 * @param c Change
 * @param d Duration
 * @returns Current animation position
 */
float Utils::expoEaseInOut(float t, float b, float c, float d) {
  if (t == 0) return b;

  if (t == d) return b + c;

  if ((t /= d / 2) < 1) return c / 2 * powf(2, 10 * (t - 1)) + b;

  return c / 2 * (-powf(2, -10 * --t) + 2) + b;
}

/** Calculates minimum and maximum X, Y, Z of mesh vertices + current position
 */
// void Utils::getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max) {
//   Vec4 calc = Vec4();
//   u8 isInitialized = 0;
//   const Vec4* BBox = t_mesh.getCurrentBoundingBox();
//   for (u32 i = 0; i < 8; i++) {
//     calc.set(BBox[i].x + t_mesh.getPosition().x,
//              BBox[i].y + t_mesh.getPosition().y,
//              BBox[i].z + t_mesh.getPosition().z);
//     if (isInitialized == 0) {
//       isInitialized = 1;
//       t_min.set(calc);
//       t_max.set(calc);
//     }

//     if (t_min.x > calc.x) t_min.x = calc.x;
//     if (calc.x > t_max.x) t_max.x = calc.x;

//     if (t_min.y > calc.y) t_min.y = calc.y;
//     if (calc.y > t_max.y) t_max.y = calc.y;

//     if (t_min.z > calc.z) t_min.z = calc.z;
//     if (calc.z > t_max.z) t_max.z = calc.z;
//   }
// }

float Utils::clamp(const float value, float min, float max) {
  return ((value < min) * (min - value)) + ((value > max) * (max - value));
}

float Utils::FOG_LINEAR(float d, float start, float end, float offset = 0.0F) {
  if (d <= offset) return end - start;
  return end - d / end - start;
}

float Utils::FOG_EXP(float d, float density) { return exp(-(d * density)); }

float Utils::FOG_EXP2(float d, float density) {
  return exp(-pow((d * density), 2));
}

float Utils::FOG_EXP_GRAD(float d, float density, float gradient) {
  return exp(-pow((d * density), gradient));
}

float Utils::Raycast(Vec4* origin, Vec4* dir, Vec4* min, Vec4* max) {
  float t1 = (min->x - origin->x) / dir->x;
  float t2 = (max->x - origin->x) / dir->x;
  float t3 = (min->y - origin->y) / dir->y;
  float t4 = (max->y - origin->y) / dir->y;
  float t5 = (min->z - origin->z) / dir->z;
  float t6 = (max->z - origin->z) / dir->z;

  float tmin = MAX(MAX(MIN(t1, t2), MIN(t3, t4)), MIN(t5, t6));
  float tmax = MIN(MIN(MAX(t1, t2), MAX(t3, t4)), MAX(t5, t6));

  // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
  if (tmax < 0) {
    return -1;
  }

  // if tmin > tmax, ray doesn't intersect AABB
  if (tmin > tmax) {
    return -1;
  }

  if (tmin < 0) {
    return tmax;
  }
  return tmin;
}