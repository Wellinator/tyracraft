#include "managers/collision_manager.hpp"

using Tyra::BBox;
using Tyra::Mesh;
using Tyra::Vec4;

CollisionManager::CollisionManager() {}

CollisionManager::~CollisionManager() {}

float CollisionManager::getManhattanDistance(const Vec4 &v1, const Vec4 &v2) {
  float x_dif, y_dif, z_dif;

  x_dif = v2.x - v1.x;
  y_dif = v2.y - v1.y;
  z_dif = v2.z - v1.z;

  if (x_dif < 0) x_dif = -x_dif;
  if (y_dif < 0) y_dif = -y_dif;
  if (z_dif < 0) z_dif = -z_dif;

  return x_dif + y_dif + z_dif;
}
