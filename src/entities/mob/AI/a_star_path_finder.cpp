#include "entities/mob/AI/a_star_path_finder.hpp"
#include "entities/mob/AI/path_result.hpp"
#include "entities/mob/AI/priority_queue.hpp"
#include "entities/level.hpp"
#include "utils.hpp"

void AStarPathFinder::TracePath(const Vec4& start, const Vec4& goal,
                                std::unordered_map<int, Vec4>* parents,
                                PathResult* path) {
  std::vector<Vec4>* list = &path->waypoints;
  Vec4 current = goal;
  Vec4 fixedWorldPos;

  // Used to fix the centroid position of the block to its bottom
  const Vec4 FixHeight(0.0f, 0.5f, 0.0f);

  while (current.x != start.x || current.y != start.y || current.z != start.z) {
    current = (*parents)[HashOffset(current)];

    fixedWorldPos = ((current - FixHeight) * DUBLE_BLOCK_SIZE);
    list->emplace(list->begin(), fixedWorldPos);
  }

  fixedWorldPos = ((goal - FixHeight) * DUBLE_BLOCK_SIZE);
  list->emplace_back(fixedWorldPos);

#ifdef DEBUG_MODE
  TYRA_LOG("AStarPathFinder::TracePath START");
  for (size_t i = 0; i < list->size(); i++) {
    (*list)[i].print("TracePath: ");
  }
  TYRA_LOG("AStarPathFinder::TracePath END");
#endif
}

bool AStarPathFinder::CanOccupyVoxel(const Vec4& voxel) {
  if (!pLevel->BoundCheckMap(voxel.x, voxel.y, voxel.z)) return false;

  int id = pLevel->GetBlockFromMap(voxel.x, voxel.y, voxel.z);
  if (id == (int)Blocks::AIR_BLOCK || id == (int)Blocks::GRASS) {
    return true;
  }

  if (id == (int)Blocks::GRASS || id == (int)Blocks::POPPY_FLOWER ||
      id == (int)Blocks::DANDELION_FLOWER) {
    return true;
  }

  return false;
}

Vec4 AStarPathFinder::GetNeighbors(const Vec4& current, int index) {
  if (index < 4) {
    int i = index;
    Vec4 next = neighbors[i] + current;
    if (CanOccupyVoxel(next)) {
      return next;
    }

    return current;
  }

  int i = index - 4;
  Vec4* pair = const_cast<Vec4*>(diagonalNeighbors[i]);
  Vec4 next = pair[0] + pair[1] + current;

  if (CanOccupyVoxel(next) && CanOccupyVoxel(pair[0] + current) &&
      CanOccupyVoxel(pair[1] + current)) {
    return next;
  }

  return current;
}

bool AStarPathFinder::FindPath(const Vec4& start, const Vec4& goal,
                               PathResult* result) {
  std::unordered_map<int, Vec4> parents;
  std::unordered_map<int, float> costs;
  std::unordered_set<int> closedset;
  PriorityQueue<Vec4> openset;

  openset.enqueue(start, 0.0f);
  int startHash = HashOffset(start);
  parents[startHash] = start;
  costs[startHash] = start.distanceTo(goal);

  while (openset.count() > 0 && openset.count() < IterationsLimit) {
    const Vec4 current = openset.dequeue();

    if (current.x == goal.x && current.y == goal.y && current.z == goal.z) {
      TracePath(start, goal, &parents, result);
      return true;
    }

    int hashCurrent = HashOffset(current);
    closedset.insert(hashCurrent);

    // Horizontal check; Y is at the same entity Y value;
    for (size_t i = 0; i < 8; i++) {
      Vec4 next = GetNeighbors(current, i);
      int hashNext = HashOffset(next);

      // If next is equal to current, next is invalid. Try vertical check;
      if (closedset.count(hashNext) || hashNext == hashCurrent) {
        // For i < 4, it's perpendicular only check
        if (i < 4) {
          const Vec4 invalidNext = current + neighbors[i];
          const Vec4 upperOffset = current + UP_VEC;
          const Vec4 nextUpper = invalidNext + UP_VEC;

          // If upperOffset is valid, it can jump up;
          if (CanOccupyVoxel(upperOffset) && !CanOccupyVoxel(invalidNext)) {
            // Check if the nextUpper is valid;
            if (CanOccupyVoxel(nextUpper)) {
              int hashNextUpper = HashOffset(nextUpper);
              if (closedset.count(hashNextUpper)) continue;

              const int multiplier = 2.0f * std::abs(nextUpper.y - current.y);
              int cost =
                  (int)(costs[hashCurrent] + current.distanceTo(nextUpper)) *
                  multiplier;

              if (!costs[hashNextUpper] || cost < costs[hashNextUpper]) {
                costs[hashNextUpper] = cost;
                float priority = cost + nextUpper.distanceTo(goal);
                openset.enqueue(nextUpper, priority);
                parents[hashNextUpper] = current;
              }
            }
          }
        }

        continue;
      };

      int cost = (int)(costs[hashCurrent] + current.distanceTo(next));

      if (!costs[hashNext] || cost < costs[hashNext]) {
        costs[hashNext] = cost;
        float priority = cost + next.distanceTo(goal);
        openset.enqueue(next, priority);
        parents[hashNext] = current;
      }
    }
  }

  return false;
}