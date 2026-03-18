#include "managers/world_generation/features/ore_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>
#include <math.h>

bool OreFeature::place(Level* level, int x, int y, int z) {
    const float direction = ((float)rand() / (float)RAND_MAX) * M_PI;

    const float startX = (float)x + 8.0f + sinf(direction) * (float)count / 8.0f;
    const float endX = (float)x + 8.0f - sinf(direction) * (float)count / 8.0f;
    const float startZ = (float)z + 8.0f + cosf(direction) * (float)count / 8.0f;
    const float endZ = (float)z + 8.0f - cosf(direction) * (float)count / 8.0f;

    const float startY = (float)(y + (rand() % 3) - 2);
    const float endY = (float)(y + (rand() % 3) - 2);

    for (int d = 0; d <= count; d++) {
        const float delta = (float)d / (float)count;
        const float centerX = startX + (endX - startX) * delta;
        const float centerY = startY + (endY - startY) * delta;
        const float centerZ = startZ + (endZ - startZ) * delta;

        const float radiusScale = ((float)rand() / (float)RAND_MAX) * (float)count / 16.0f;
        const float horizontalRadius = (sinf(delta * M_PI) + 1.0f) * radiusScale + 1.0f;
        const float verticalRadius = (sinf(delta * M_PI) + 1.0f) * radiusScale + 1.0f;

        const float halfHR = horizontalRadius * 0.5f;
        const float halfVR = verticalRadius * 0.5f;
        const float invHalfHR = 1.0f / halfHR;
        const float invHalfVR = 1.0f / halfVR;

        const int minX = (int)floorf(centerX - halfHR);
        const int minY = (int)floorf(centerY - halfVR);
        const int minZ = (int)floorf(centerZ - halfHR);

        const int maxX = (int)floorf(centerX + halfHR);
        const int maxY = (int)floorf(centerY + halfVR);
        const int maxZ = (int)floorf(centerZ + halfHR);

        for (int curX = minX; curX <= maxX; curX++) {
            const float dx = ((float)curX + 0.5f - centerX) * invHalfHR;
            if (dx * dx < 1.0f) {
                for (int curY = minY; curY <= maxY; curY++) {
                    const float dy = ((float)curY + 0.5f - centerY) * invHalfVR;
                    if (dx * dx + dy * dy < 1.0f) {
                        for (int curZ = minZ; curZ <= maxZ; curZ++) {
                            const float dz = ((float)curZ + 0.5f - centerZ) * invHalfHR;
                            if (dx * dx + dy * dy + dz * dz < 1.0f) {
                                if (level->BoundCheckMap(curX, curY, curZ) && 
                                    level->GetBlockFromMap(curX, curY, curZ) == targetBlockType) {
                                    placeBlock(level, curX, curY, curZ, oreBlockType);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}
