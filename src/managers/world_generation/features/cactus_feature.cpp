#include "managers/world_generation/features/cactus_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

static bool isAirBlock(Level* level, const int x, const int y, const int z) {
    if (!level->BoundCheckMap(x, y, z)) {
        return false;
    }

    return level->GetBlockFromMap(x, y, z) ==
           static_cast<uint8_t>(Blocks::AIR_BLOCK);
}

bool CactusFeature::place(Level* level, int x, int y, int z) {
    (void)y;

    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t sandBlockId = static_cast<uint8_t>(Blocks::SAND_BLOCK);
    const uint8_t cactusBlockId = static_cast<uint8_t>(Blocks::CACTUS_BLOCK);

    for (int i = 0; i < 10; i++) {
        const int randX = x + (rand() % 8) - (rand() % 8);
        const int randZ = z + (rand() % 8) - (rand() % 8);

        if (!level->BoundCheckMap(randX, 0, randZ)) {
            continue;
        }

        int surfaceY = level->map.height - 1;
        while (surfaceY > 0 &&
               level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId) {
            surfaceY--;
        }
        surfaceY += 1;

        if (!level->BoundCheckMap(randX, surfaceY, randZ) ||
            !level->BoundCheckMap(randX, surfaceY - 1, randZ)) {
            continue;
        }

        const uint8_t blockBelow = level->GetBlockFromMap(randX, surfaceY - 1, randZ);
        if (blockBelow != sandBlockId) {
            continue;
        }

        // Check if the center and neighbors are air (Standard MC cactus rule: no adjacent blocks)
        if (!isAirBlock(level, randX, surfaceY, randZ) ||
            !isAirBlock(level, randX - 1, surfaceY, randZ) ||
            !isAirBlock(level, randX + 1, surfaceY, randZ) ||
            !isAirBlock(level, randX, surfaceY, randZ - 1) ||
            !isAirBlock(level, randX, surfaceY, randZ + 1)) {
            continue;
        }

        const int cactusHeight = 1 + (rand() % 3);
        for (int offsetY = 0; offsetY < cactusHeight; offsetY++) {
            const int currentY = surfaceY + offsetY;
            
            // Re-check air for each block in the column (in case something changed or for height limit)
            if (!isAirBlock(level, randX, currentY, randZ)) {
                break;
            }

            placeBlock(level, randX, currentY, randZ, cactusBlockId);
        }
    }

    return true;
}
