/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "utils.hpp"
#include <fastmath.h>
#include <physics/ray.hpp>
#include <renderer/3d/bbox/bbox.hpp>
#include <sifrpc.h>
#include <loadfile.h>
#include <libvux.h>

using Tyra::BBox;
using Tyra::Color;
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

float Utils::clamp(const float value, float min, float max) {
  return ((value < min) * (min - value)) + ((value > max) * (max - value));
}

float Utils::FOG_LINEAR(const float& d, const float& start, const float& end,
                        const float& offset) {
  if (d <= offset) return (end - start) * 1 / end;
  return (end - d / end - start) * 1 / end;
}

float Utils::FOG_EXP(float d, float density) {
  return expf_fast(-(d * density));
}

float Utils::FOG_EXP2(float d, float density) {
  return exp(-pow((d * density), 2));
}

float Utils::FOG_EXP_GRAD(float d, float density, float gradient) {
  return expf_fast(-fastPow((d * density), gradient));
}

float Utils::fastPow(float a, float b) {
  union {
    float d;
    int x[2];
  } u = {a};
  u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;
  return u.d;
}

float Utils::expf_fast(float a) {
  union {
    float f;
    int x;
  } u;
  u.x = (int)(12102203 * a + 1064866805);
  return u.f;
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
    return -1.0f;
  }

  // if tmin > tmax, ray doesn't intersect AABB
  if (tmin > tmax) {
    return -1.0f;
  }

  if (tmin < 0) {
    return tmax;
  }
  return tmin;
}

void Utils::GetMinkowskiSum(const Vec4& AMin, const Vec4& AMax,
                            const Vec4& BMin, const Vec4& BMax, Vec4* resultMin,
                            Vec4* resultMax) {
  // Width, Height, Depth
  Vec4 aDimensions = Vec4(AMax - AMin) / 2;
  resultMax->set(aDimensions + BMax);
  resultMin->set(BMin - aDimensions);
}

Vec4 Utils::GetNormalFromHitPosition(const Vec4& intersection,
                                     const Vec4& boxMin, const Vec4& boxMax) {
  Vec4 normal = Vec4(0, 0, 0);
  if (intersection.z == boxMax.z) {
    normal.set(0, 0, -1);
  } else if (intersection.z == boxMin.z) {
    normal.set(0, 0, 1);
  } else if (intersection.x == boxMax.x) {
    normal.set(1, 0, 0);
  } else if (intersection.x == boxMin.x) {
    normal.set(-1, 0, 0);
  } else if (intersection.y == boxMax.y) {
    normal.set(0, 1, 0);
  } else if (intersection.y == boxMin.y) {
    normal.set(0, -1, 0);
  }
  return normal;
}

/**
 * @brief Test BBox againt Frustum Planes
 * @details https://gist.github.com/Kinwailo/d9a07f98d8511206182e50acda4fbc9b
 *
 */
CoreBBoxFrustum Utils::FrustumAABBIntersect(const Plane* frustumPlanes,
                                            const BBox& AABB) {
  Vec4 mins, maxs;
  AABB.getMinMax(&mins, &maxs);
  return Utils::FrustumAABBIntersect(frustumPlanes, &mins, &maxs);
}

CoreBBoxFrustum Utils::FrustumAABBIntersect(const Plane* frustumPlanes,
                                            const Vec4* mins,
                                            const Vec4* maxs) {
  CoreBBoxFrustum result = CoreBBoxFrustum::IN_FRUSTUM;
  Vec4 vmin, vmax;

  for (u8 i = 0; i < 6; ++i) {
    // X axis
    if (frustumPlanes[i].normal.x < 0) {
      vmin.x = mins->x;
      vmax.x = maxs->x;
    } else {
      vmin.x = maxs->x;
      vmax.x = mins->x;
    }
    // Y axis
    if (frustumPlanes[i].normal.y < 0) {
      vmin.y = mins->y;
      vmax.y = maxs->y;
    } else {
      vmin.y = maxs->y;
      vmax.y = mins->y;
    }
    // Z axis
    if (frustumPlanes[i].normal.z < 0) {
      vmin.z = mins->z;
      vmax.z = maxs->z;
    } else {
      vmin.z = maxs->z;
      vmax.z = mins->z;
    }

    // float A = frustumPlanes[i].normal.x * vmin.x +
    //           frustumPlanes[i].normal.y * vmin.y +
    //           frustumPlanes[i].normal.z * vmin.z + frustumPlanes[i].distance;
    if (frustumPlanes[i].normal.dot3(vmin) + frustumPlanes[i].distance < 0)
      return CoreBBoxFrustum::OUTSIDE_FRUSTUM;

    // float B = frustumPlanes[i].normal.x * vmax.x +
    //           frustumPlanes[i].normal.y * vmax.y +
    //           frustumPlanes[i].normal.z * vmax.z + frustumPlanes[i].distance;

    if (frustumPlanes[i].normal.dot3(vmax) + frustumPlanes[i].distance <= 0)
      result = CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM;
  }

  return result;
}

std::vector<UtilDirectory> Utils::listDir(const char* dir) {
  std::vector<UtilDirectory> result;

  DIR* dirp = opendir(dir);
  dirent* dp;
  while ((dp = readdir(dirp)) != NULL) {
    result.push_back(UtilDirectory(dp));
  }
  closedir(dirp);

  return result;
}

std::vector<UtilDirectory> Utils::listDir(const std::string& dir) {
  return Utils::listDir(dir.c_str());
}

std::string Utils::trim(std::string& str) {
  str.erase(str.find_last_not_of(' ') + 1);
  str.erase(0, str.find_first_not_of(' '));
  return str;
}

bool Utils::AABBCollides(BBox* A, BBox* B) {
  Vec4 minA, maxA, minB, maxB;
  A->getMinMax(&minA, &maxA);
  B->getMinMax(&minB, &maxB);

  return minA.x <= maxB.x && maxA.x >= minB.x && minA.y <= maxB.y &&
         maxA.y >= minB.y && minA.z <= maxB.z && maxA.z >= minB.z;
}

bool Utils::AABBCollidesXZ(BBox* A, BBox* B) {
  Vec4 minA, maxA, minB, maxB;
  A->getMinMax(&minA, &maxA);
  B->getMinMax(&minB, &maxB);

  return minA.x <= maxB.x && maxA.x >= minB.x && minA.z <= maxB.z &&
         maxA.z >= minB.z;
}

/* Function to get no of set bits in binary
representation of positive integer n */
u8 Utils::countSetBits(u32 n) {
  u8 count = 0;
  while (n) {
    count += n & 1;
    n >>= 1;
  }
  return count;
}

float Utils::Abs(const float x) {
  float r;
  asm(" abs.s %0, %1 " : "=&f"(r) : "f"(x));
  return r;
}

void Utils::inverseMatrix(M4x4* mOut, const M4x4* mIn) {
    // Inverse of input matrix.
    // This was salvaged from libVu0.c
    asm volatile(
        "lq           $8,   0x00(%1) \n\t"
        "lq           $9,   0x10(%1) \n\t"
        "lq           $10,  0x20(%1) \n\t"
        "lqc2         $vf4, 0x30(%1) \n\t"
        "vmove.xyzw   $vf5, $vf4 \n\t"
        "vsub.xyz     $vf4, $vf4, $vf4 \n\t"
        "vmove.xyzw   $vf9, $vf4 \n\t"
        "qmfc2        $11,  $vf4 \n\t"
        "pextlw       $12,  $9,  $8 \n\t"
        "pextuw       $13,  $9,  $8 \n\t"
        "pextlw       $14,  $11, $10 \n\t"
        "pextuw       $15,  $11, $10 \n\t"
        "pcpyld       $8,   $14, $12 \n\t"
        "pcpyud       $9,   $12, $14 \n\t"
        "pcpyld       $10,  $15, $13 \n\t"
        "qmtc2        $8,   $vf6 \n\t"
        "qmtc2        $9,   $vf7 \n\t"
        "qmtc2        $10,  $vf8 \n\t"
        "vmulax.xyz   $ACC, $vf6, $vf5 \n\t"
        "vmadday.xyz  $ACC, $vf7, $vf5 \n\t"
        "vmaddz.xyz   $vf4, $vf8, $vf5 \n\t"
        "vsub.xyz     $vf4, $vf9, $vf4 \n\t"
        "sq           $8,   0x00(%0) \n\t"
        "sq           $9,   0x10(%0) \n\t"
        "sq           $10,  0x20(%0) \n\t"
        "sqc2         $vf4, 0x30(%0) \n\t"
        :
        : "r"(mOut->data), "r"(mIn->data));
  }