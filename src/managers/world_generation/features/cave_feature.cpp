#include "managers/world_generation/features/cave_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>
#include <math.h>
#include <vector>

struct CaveVoxel {
  int x, y, z;
  CaveVoxel(int x, int y, int z) : x(x), y(y), z(z) {}
};

bool CaveFeature::place(Level* level, int x, int y, int z) {
  // Original Minecraft segment-based cave generation
  const float direction = ((float)rand() / (float)RAND_MAX) * M_PI;
  const float radiusOffset = 8.0f;

  const float startX = (float)x + 8.0f + sinf(direction) * radiusOffset;
  const float endX = (float)x + 8.0f - sinf(direction) * radiusOffset;
  const float startZ = (float)z + 8.0f + cosf(direction) * radiusOffset;
  const float endZ = (float)z + 8.0f - cosf(direction) * radiusOffset;

  const float startY = (float)(y + (rand() % 8) + 2);
  const float endY = (float)(y + (rand() % 8) + 2);

  const float baseRadius = ((float)rand() / (float)RAND_MAX) * 4.0f + 2.0f;
  const float noiseFactor = ((float)rand() / (float)RAND_MAX) * 0.6f;

  const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
  const uint8_t waterBlockId = static_cast<uint8_t>(Blocks::WATER_BLOCK);
  const uint8_t lavaBlockId = static_cast<uint8_t>(Blocks::LAVA_BLOCK);
  const uint8_t dirtyBlockId = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
  const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);

  std::vector<CaveVoxel> voxelsToRemove;

  for (int step = 0; step <= 16; step++) {
    const float delta = (float)step / 16.0f;
    const float centerX = startX + (endX - startX) * delta;
    const float centerY = startY + (endY - startY) * delta;
    const float centerZ = startZ + (endZ - startZ) * delta;

    const float radiusScale = (float)rand() / (float)RAND_MAX;
    const float horizontalRadius = (sinf(delta * M_PI) * baseRadius + 1.0f) * radiusScale + 1.0f;
    const float verticalRadius = (sinf(delta * M_PI) * baseRadius + 1.0f) * radiusScale + 1.0f;

    const float halfHR = horizontalRadius * 0.5f;
    const float halfVR = verticalRadius * 0.5f;
    const float invHalfHR = 1.0f / halfHR;
    const float invHalfVR = 1.0f / halfVR;

    const int minX = (int)(centerX - halfHR);
    const int maxX = (int)(centerX + halfHR);
    const int minY = (int)(centerY - halfVR);
    const int maxY = (int)(centerY + halfVR);
    const int minZ = (int)(centerZ - halfHR);
    const int maxZ = (int)(centerZ + halfHR);

    for (int curX = minX; curX <= maxX; curX++) {
      for (int curY = minY; curY <= maxY; curY++) {
        for (int curZ = minZ; curZ <= maxZ; curZ++) {
          const float dx = ((float)curX + 0.5f - centerX) * invHalfHR;
          const float dy = ((float)curY + 0.5f - centerY) * invHalfVR;
          const float dz = ((float)curZ + 0.5f - centerZ) * invHalfHR;

          if (dx * dx + dy * dy + dz * dz < ((float)rand() / (float)RAND_MAX) * noiseFactor + (1.0f - noiseFactor)) {
            if (level->BoundCheckMap(curX, curY, curZ) &&
                level->GetBlockFromMap(curX, curY, curZ) != airBlockId) {
              
              // Liquid safety check (prevent caves from flooding with water/lava)
              bool liquidNeighbor = false;
              for (int checkX = curX - 2; checkX <= curX + 1; checkX++) {
                for (int checkY = curY - 1; checkY <= curY + 1; checkY++) {
                  for (int checkZ = curZ - 1; checkZ <= curZ + 1; checkZ++) {
                    if (level->BoundCheckMap(checkX, checkY, checkZ)) {
                      const uint8_t blk = level->GetBlockFromMap(checkX, checkY, checkZ);
                      if (blk == waterBlockId || blk == lavaBlockId) {
                        liquidNeighbor = true;
                        break;
                      }
                    }
                  }
                  if (liquidNeighbor) break;
                }
                if (liquidNeighbor) break;
              }

              if (liquidNeighbor) return false;

              voxelsToRemove.emplace_back(curX, curY, curZ);
            }
          }
        }
      }
    }
  }

  for (const auto& voxel : voxelsToRemove) {
    level->SetBlockInMap(voxel.x, voxel.y, voxel.z, airBlockId);
  }

  // Restore grass if applicable (secondary pass to avoid check issues)
  for (const auto& voxel : voxelsToRemove) {
    if (level->BoundCheckMap(voxel.x, voxel.y - 1, voxel.z)) {
      if (level->GetBlockFromMap(voxel.x, voxel.y - 1, voxel.z) == dirtyBlockId &&
          level->GetLightFromMap(voxel.x, voxel.y, voxel.z) > 8) {
        level->SetBlockInMap(voxel.x, voxel.y - 1, voxel.z, grassBlockId);
      }
    }
  }

  return true;
}
