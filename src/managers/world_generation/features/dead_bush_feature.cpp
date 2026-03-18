#include "managers/world_generation/features/dead_bush_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool DeadBushFeature::place(Level* level, int x, int y, int z) {
    (void)y;

    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t sandBlockId = static_cast<uint8_t>(Blocks::SAND_BLOCK);
    const uint8_t deadBushId = static_cast<uint8_t>(Blocks::DEAD_BUSH);

    for (int i = 0; i < 4; i++) {
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
        const uint8_t atBlock = level->GetBlockFromMap(randX, surfaceY, randZ);

        if (atBlock == airBlockId && blockBelow == sandBlockId) {
            placeBlock(level, randX, surfaceY, randZ, deadBushId);
        }
    }

    return true;
}
