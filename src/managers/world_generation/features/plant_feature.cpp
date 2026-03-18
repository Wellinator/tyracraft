#include "managers/world_generation/features/plant_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool PlantFeature::place(Level* level, int x, int y, int z) {
    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
    const uint8_t grassPlantId = static_cast<uint8_t>(Blocks::GRASS);
    const uint8_t stoneBlockId = static_cast<uint8_t>(Blocks::STONE_BLOCK);

    if (type == PlantType::Grass) {
        for (int j = 0; j < 25; j++) {
            uint16_t randX = x;
            uint16_t randZ = z;

            for (int k = 0; k < 5; k++) {
                randX += (rand() % 6) - (rand() % 6);
                randZ += (rand() % 6) - (rand() % 6);

                if (level->BoundCheckMap(randX, 0, randZ)) {
                    // Start finding surface downwards
                    uint16_t surfaceY = level->map.height - 1;
                    while (surfaceY > 0 && level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId) {
                        surfaceY--;
                    }
                    surfaceY += offset;

                    if (!level->BoundCheckMap(randX, surfaceY, randZ)) continue;

                    uint8_t blockBelowId = level->GetBlockFromMap(randX, surfaceY - 1, randZ);

                    if (level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId &&
                        blockBelowId == grassBlockId) {
                        placeBlock(level, randX, surfaceY, randZ, grassPlantId);
                    }
                }
            }
        }
        return true;
    } else if (type == PlantType::Flower) {
        Blocks flowerType = (rand() % 2 == 0) ? Blocks::DANDELION_FLOWER : Blocks::POPPY_FLOWER;
        const uint8_t flowerBlockId = static_cast<uint8_t>(flowerType);

        for (int j = 0; j < 10; j++) {
            uint16_t randX = x;
            uint16_t randZ = z;

            for (int k = 0; k < 5; k++) {
                randX += (rand() % 6) - (rand() % 6);
                randZ += (rand() % 6) - (rand() % 6);

                if (level->BoundCheckMap(randX, 0, randZ)) {
                    uint16_t surfaceY = level->map.height - 1;
                    while (surfaceY > 0 && level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId) {
                        surfaceY--;
                    }
                    surfaceY += offset;

                    if (!level->BoundCheckMap(randX, surfaceY, randZ)) continue;

                    uint8_t blockBelowId = level->GetBlockFromMap(randX, surfaceY - 1, randZ);

                    if (level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId &&
                        blockBelowId == grassBlockId) {
                        placeBlock(level, randX, surfaceY, randZ, flowerBlockId);
                    }
                }
            }
        }
        return true;
    } else if (type == PlantType::Mushroom) {
        for (int j = 0; j < 20; j++) {
            uint16_t randX = x;
            uint16_t surfaceY = y;
            uint16_t randZ = z;

            for (int k = 0; k < 5; k++) {
                randX += (rand() % 6) - (rand() % 6);
                surfaceY += (rand() % 2) - (rand() % 2);
                randZ += (rand() % 6) - (rand() % 6);

                if (level->BoundCheckMap(randX, surfaceY, randZ) && level->BoundCheckMap(randX, surfaceY - 1, randZ)) {
                    uint8_t blockBelowId = level->GetBlockFromMap(randX, surfaceY - 1, randZ);

                    if (level->GetBlockFromMap(randX, surfaceY, randZ) == airBlockId &&
                        blockBelowId == stoneBlockId) {
                        // Original code has mushrooms commented out, just clearing air for now
                        placeBlock(level, randX, surfaceY, randZ, airBlockId);
                    }
                }
            }
        }
        return true;
    }

    return false;
}
