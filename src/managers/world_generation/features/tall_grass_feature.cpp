#include "managers/world_generation/features/tall_grass_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool TallGrassFeature::place(Level* level, int x, int y, int z) {
    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t oakLeavesBlockId = static_cast<uint8_t>(Blocks::OAK_LEAVES_BLOCK);
    const uint8_t birchLeavesBlockId = static_cast<uint8_t>(Blocks::BIRCH_LEAVES_BLOCK);
    const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
    const uint8_t dirtyBlockId = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
    const uint8_t tallGrassId = static_cast<uint8_t>(Blocks::TALL_GRASS_BLOCK);

    // Traverse down until a solid block is found (skip air, leaves)
    while (y > 0) {
        if (!level->BoundCheckMap(x, y, z)) return false;
        
        const uint8_t atBlockId = level->GetBlockFromMap(x, y, z);
        if (atBlockId != airBlockId && atBlockId != oakLeavesBlockId && atBlockId != birchLeavesBlockId) {
            break;
        }
        y--;
    }

    for (int i = 0; i < 128; i++) {
        int randX = x + (rand() % 8) - (rand() % 8);
        int randY = y + (rand() % 4) - (rand() % 4);
        int randZ = z + (rand() % 8) - (rand() % 8);

        if (!level->BoundCheckMap(randX, randY, randZ) || !level->BoundCheckMap(randX, randY - 1, randZ)) {
            continue;
        }

        if (level->GetBlockFromMap(randX, randY, randZ) == airBlockId) {
            // Check if block can survive here
            uint8_t blockBelowId = level->GetBlockFromMap(randX, randY - 1, randZ);
            if (blockBelowId == grassBlockId || blockBelowId == dirtyBlockId) {
                if (grassBlockType == tallGrassId) {
                    // Check if there is space above for the upper half
                    if (randY + 1 < (int)level->map.height && 
                        level->GetBlockFromMap(randX, randY + 1, randZ) == airBlockId) {
                        placeBlock(level, randX, randY, randZ, grassBlockType);
                        level->ResetIsUpperHalfDataToMap(randX, randY, randZ); // Lower half

                        placeBlock(level, randX, randY + 1, randZ, grassBlockType);
                        level->SetIsUpperHalfDataToMap(randX, randY + 1, randZ, true); // Upper half
                    }
                } else {
                    // Normal grass plant (single block)
                    placeBlock(level, randX, randY, randZ, grassBlockType);
                }
            }
        }
    }

    return true;
}
