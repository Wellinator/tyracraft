#pragma once
#include <tamtypes.h>
#include <vector>
#include "camera.hpp"
#include "singleton.hpp"
#include "tyra"

using Tyra::Vec4;

class FastVoxelTraversalService : public Singleton<FastVoxelTraversalService> {
 public:
  void voxelTraversalAll(const Vec4& ray_start, const Vec4& ray_end,
                         std::vector<Vec4>* pResult);
};
