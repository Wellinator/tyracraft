#include "managers/world_generation/features/reeds_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool ReedsFeature::place(Level* level, int x, int y, int z) {
    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t waterBlockId = static_cast<uint8_t>(Blocks::WATER_BLOCK);
    const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
    const uint8_t dirtyBlockId = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
    const uint8_t sandBlockId = static_cast<uint8_t>(Blocks::SAND_BLOCK);

    for (int i = 0; i < 20; i++) {
        int randX = x + (rand() % 4) - (rand() % 4);
        int surfaceY = y;
        int randZ = z + (rand() % 4) - (rand() % 4);

        if (!level->BoundCheckMap(randX, surfaceY, randZ)) continue;

        if (level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId) {
            // Find surface
            while (surfaceY > 0 && level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId) {
                surfaceY--;
            }
            surfaceY++; // return to air block above surface

            if (!level->BoundCheckMap(randX, surfaceY, randZ)) continue;

            bool nearWater = false;
            
            auto checkWater = [&](int cx, int cy, int cz) {
                if (level->BoundCheckMap(cx, cy, cz)) {
                    if (level->GetBlockFromMap(cx, cy, cz) == waterBlockId) {
                        return true;
                    }
                }
                return false;
            };

            // Needs to be adjacent to water horizontally directly next to the placement block
            if (checkWater(randX - 1, surfaceY - 1, randZ) || 
                checkWater(randX + 1, surfaceY - 1, randZ) || 
                checkWater(randX, surfaceY - 1, randZ - 1) || 
                checkWater(randX, surfaceY - 1, randZ + 1)) {
                
                nearWater = true;
            }

            if (nearWater) {
                if (level->BoundCheckMap(randX, surfaceY - 1, randZ)) {
                    uint8_t blockBelowId = level->GetBlockFromMap(randX, surfaceY - 1, randZ);
                    if (blockBelowId == grassBlockId || 
                        blockBelowId == dirtyBlockId || 
                        blockBelowId == sandBlockId) {
                        
                        int reedHeight = 2 + (rand() % ((rand() % 3) + 1));
                        for (int h = 0; h < reedHeight; h++) {
                            if (level->BoundCheckMap(randX, surfaceY + h, randZ)) {
                                if (level->GetBlockFromMap(randX, surfaceY + h, randZ) == airBlockId) {
                                    placeBlock(level, randX, surfaceY + h, randZ, reedsBlockType);
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
