#include "managers/world_generation/features/sand_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

namespace TyraCraft {

SandFeature::SandFeature(int t_radius, uint8_t t_tile) {
  tile = t_tile;
  radius = t_radius;
}

bool SandFeature::place(Level* level, int x, int y, int z) {
  const uint8_t waterBlockId = static_cast<uint8_t>(Blocks::WATER_BLOCK);
  const uint8_t dirtyBlockId = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
  const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);

  if (level->SafeGetBlockFromMap(x, y, z) != waterBlockId) {
    return false;
  }

  int currRadius = (rand() % (radius - 2)) + 2;
  int verticalRadius = 2;

  for (int currX = x - currRadius; currX <= x + currRadius; currX++) {
    for (int currZ = z - currRadius; currZ <= z + currRadius; currZ++) {
      int distX = currX - x;
      int distZ = currZ - z;
      if (distX * distX + distZ * distZ > currRadius * currRadius) continue;

      for (int currY = y - verticalRadius; currY <= y + verticalRadius; currY++) {
        if (!level->BoundCheckMap(currX, currY, currZ)) continue;

        const uint8_t atBlockId = level->GetBlockFromMap(currX, currY, currZ);
        if (atBlockId == dirtyBlockId || atBlockId == grassBlockId) {
          placeBlock(level, currX, currY, currZ, tile);
        }
      }
    }
  }

  return true;
}

}  // namespace TyraCraft
