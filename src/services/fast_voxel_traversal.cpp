#include "services/fast_voxel_traversal.hpp"
#include "entities/level.hpp"
#include <float.h>

void FastVoxelTraversalService::voxelTraversalAll(const Vec4& ray_start,
                                                  const Vec4& ray_end,
                                                  std::vector<Vec4>* pResult) {
  // This id of the first/current voxel hit by the ray.
  // Using floor (round down) is actually very important,
  // the implicit int-casting will round up for negative numbers.
  Vec4 current_voxel(std::floor(ray_start.x / DOUBLE_BLOCK_SIZE),
                     std::floor(ray_start.y / DOUBLE_BLOCK_SIZE),
                     std::floor(ray_start.z / DOUBLE_BLOCK_SIZE));

  // The id of the last voxel hit by the ray.
  // TODO: what happens if the end point is on a border?
  Vec4 last_voxel(std::floor(ray_end.x / DOUBLE_BLOCK_SIZE),
                  std::floor(ray_end.y / DOUBLE_BLOCK_SIZE),
                  std::floor(ray_end.z / DOUBLE_BLOCK_SIZE));

  // Compute normalized ray direction.
  Vec4 ray = (ray_end - ray_start).getNormalized();

  // In which direction the voxel ids are incremented.
  float stepX = (ray.x >= 0) ? 1 : -1;
  float stepY = (ray.y >= 0) ? 1 : -1;
  float stepZ = (ray.z >= 0) ? 1 : -1;

  // Distance along the ray to the next voxel border from the current position
  // (tMaxX, tMaxY, tMaxZ).
  float next_voxel_boundary_x = (current_voxel.x + stepX) * DOUBLE_BLOCK_SIZE;
  float next_voxel_boundary_y = (current_voxel.y + stepY) * DOUBLE_BLOCK_SIZE;
  float next_voxel_boundary_z = (current_voxel.z + stepZ) * DOUBLE_BLOCK_SIZE;

  // tMaxX, tMaxY, tMaxZ -- distance until next intersection with voxel-border
  // the value of t at which the ray crosses the first vertical voxel boundary
  float tMaxX = (ray.x != 0) ? (next_voxel_boundary_x - ray_start.x) / ray.x
                             : FLT_MAX;  //
  float tMaxY = (ray.y != 0) ? (next_voxel_boundary_y - ray_start.y) / ray.y
                             : FLT_MAX;  //
  float tMaxZ = (ray.z != 0) ? (next_voxel_boundary_z - ray_start.z) / ray.z
                             : FLT_MAX;  //

  // tDeltaX, tDeltaY, tDeltaZ --
  // how far along the ray we must move for the horizontal component to equal
  // the width of a voxel the direction in which we traverse the grid can only
  // be FLT_MAX if we never go in that direction
  float tDeltaX = (ray.x != 0) ? DOUBLE_BLOCK_SIZE / ray.x * stepX : FLT_MAX;
  float tDeltaY = (ray.y != 0) ? DOUBLE_BLOCK_SIZE / ray.y * stepY : FLT_MAX;
  float tDeltaZ = (ray.z != 0) ? DOUBLE_BLOCK_SIZE / ray.z * stepZ : FLT_MAX;

  Vec4 diff(0, 0, 0);
  bool neg_ray = false;

  if (current_voxel.x != last_voxel.x && ray.x < 0) {
    diff.x--;
    neg_ray = true;
  }
  if (current_voxel.y != last_voxel.y && ray.y < 0) {
    diff.y--;
    neg_ray = true;
  }
  if (current_voxel.z != last_voxel.z && ray.z < 0) {
    diff.z--;
    neg_ray = true;
  }

  pResult->emplace_back(current_voxel);

  if (neg_ray) {
    current_voxel += diff;
    pResult->emplace_back(current_voxel);
  }

  while ((last_voxel - current_voxel).length() != 0.0f) {
    if (tMaxX < tMaxY) {
      if (tMaxX < tMaxZ) {
        current_voxel.x += stepX;
        tMaxX += tDeltaX;
      } else {
        current_voxel.z += stepZ;
        tMaxZ += tDeltaZ;
      }
    } else {
      if (tMaxY < tMaxZ) {
        current_voxel.y += stepY;
        tMaxY += tDeltaY;
      } else {
        current_voxel.z += stepZ;
        tMaxZ += tDeltaZ;
      }
    }
    pResult->emplace_back(current_voxel);
  }
}
