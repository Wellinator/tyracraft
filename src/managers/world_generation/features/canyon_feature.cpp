#include "managers/world_generation/features/canyon_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>
#include <math.h>

void CanyonFeature::fillOblateSpheroid(Level* level, int centerX, int centerY,
                                       int centerZ, float radX, float radY,
                                       float radZ, uint8_t blockId) {
  const uint8_t stoneBlock = static_cast<uint8_t>(Blocks::STONE_BLOCK);
  const uint8_t dirtyBlock = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
  const uint8_t grassBlock = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
  const uint8_t lavaBlock = static_cast<uint8_t>(Blocks::LAVA_BLOCK);
  const uint8_t percent100 = static_cast<uint8_t>(LiquidLevel::Percent100);

  const float invRadX = 1.0f / radX;
  const float invRadY = 1.0f / radY;
  const float invRadZ = 1.0f / radZ;

  for (int x = centerX - static_cast<int>(radX);
       x <= centerX + static_cast<int>(radX); x++) {
    for (int y = centerY - static_cast<int>(radY);
         y <= centerY + static_cast<int>(radY); y++) {
      for (int z = centerZ - static_cast<int>(radZ);
           z <= centerZ + static_cast<int>(radZ); z++) {
        if (level->BoundCheckMap(x, y, z)) {
          float dx = (x + 0.5f - centerX) * invRadX;
          float dy = (y + 0.5f - centerY) * invRadY;
          float dz = (z + 0.5f - centerZ) * invRadZ;
          float distSq = dx * dx + dy * dy + dz * dz;

          if (distSq < 1.0f) {
            uint8_t currentBlock = level->GetBlockFromMap(x, y, z);
            if (currentBlock == stoneBlock || currentBlock == dirtyBlock ||
                currentBlock == grassBlock) {
              if (y < 10) {
                placeBlock(level, x, y, z, lavaBlock);
                level->SetLiquidDataToMap(x, y, z, percent100);
              } else {
                placeBlock(level, x, y, z, blockId);
              }
            }
          }
        }
      }
    }
  }
}

bool CanyonFeature::place(Level* level, int x, int y, int z) {
  // Canyon features are winding paths like caves but deeper and narrower.
  int currentX = x;
  int currentY = y;
  int currentZ = z;

  // Canyons are usually longer than caves in TyraCraft's implementation
  int canyonLength =
      (int)((rand() / (float)RAND_MAX + rand() / (float)RAND_MAX) * 150.0f) + 50;

  float theta = (rand() / (float)RAND_MAX) * M_PI * 2.0f;
  float deltaTheta = 0.0f;
  float phi = ((rand() / (float)RAND_MAX) - 0.5f) * 0.25f;  // Less verticality
  float deltaPhi = 0.0f;

  float baseRadius = (rand() / (float)RAND_MAX * 2.0f + 1.0f);

  const uint8_t airBlock = static_cast<uint8_t>(Blocks::AIR_BLOCK);

  for (int len = 0; len < canyonLength; len++) {
    currentX += (int)(sinf(theta) * cosf(phi) * 1.5f);
    currentY += (int)(sinf(phi) * 1.5f);
    currentZ += (int)(cosf(theta) * cosf(phi) * 1.5f);

    theta += deltaTheta * 0.1f;
    deltaTheta = (deltaTheta * 0.5f) +
                  (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX) * 0.1f;
    phi = phi * 0.8f + (rand() / (float)RAND_MAX - rand() / (float)RAND_MAX) * 0.05f;

    if (rand() / (float)RAND_MAX >= 0.15f) {
      float rad = 1.5f + sinf(len * M_PI / canyonLength) * baseRadius * 2.0f;
      float radY = rad * 3.0f; // Characteristic vertical scale of ravines
      float radX = rad * (0.75f + (rand() / (float)RAND_MAX) * 0.25f);
      float radZ = rad * (0.75f + (rand() / (float)RAND_MAX) * 0.25f);

      fillOblateSpheroid(level, currentX, currentY, currentZ, radX, radY, radZ, airBlock);
    }
  }

  return true;
}
