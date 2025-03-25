#pragma once

#include <tyra>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "entities/level.hpp"
#include "singleton.hpp"
#include "path_result.hpp"
#include "draw_types.h"
#include "managers/chunk_manager.hpp"

using Tyra::Vec4;

class AStarPathFinder {
 public:
  void TracePath(const Vec4& start, const Vec4& goal,
                 std::unordered_map<int, Vec4>* parents, PathResult* path);
  bool CanOccupyVoxel(const Vec4& voxel);
  Vec4 GetNeighbors(const Vec4& current, int index);
  bool FindPath(const Vec4& start, const Vec4& goal, PathResult* result);

 private:
  ChunkManager* pChunkManager = ChunkManager::getInstance();
  Level* pLevel = Level::getInstance();

  const size_t IterationsLimit = 100;

  int HashVec(const Vec4* vec) {
    int result = ftoi4(vec->x);
    result = (result * 397) ^ ftoi4(vec->y);
    result = (result * 397) ^ ftoi4(vec->z);
    return result;
  }

  int HashOffset(const Vec4& offset) {
    return pLevel->GetPosFromXYZ(offset.x, offset.y, offset.z);
  }

  const Vec4 neighbors[4] = {
      Vec4(1, 0, 0),   // North
      Vec4(0, 0, 1),   // East
      Vec4(-1, 0, 0),  // South
      Vec4(0, 0, -1),  // West
  };

  const Vec4 diagonalNeighbors[4][2] = {
      {Vec4(1, 0, 0), Vec4(0, 0, 1)},   // North - East
      {Vec4(1, 0, 0), Vec4(0, 0, -1)},  // North - West
      {Vec4(-1, 0, 0), Vec4(0, 0, 1)},  // South - East
      {Vec4(-1, 0, 0), Vec4(0, 0, -1)}  // South - West
  };
};
