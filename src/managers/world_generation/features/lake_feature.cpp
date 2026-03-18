#include "managers/world_generation/features/lake_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool LakeFeature::place(Level* level, int x, int y, int z) {
    x -= 8;
    z -= 8;

    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t waterBlockId = static_cast<uint8_t>(Blocks::WATER_BLOCK);
    const uint8_t lavaBlockId = static_cast<uint8_t>(Blocks::LAVA_BLOCK);
    const uint8_t dirtyBlockId = static_cast<uint8_t>(Blocks::DIRTY_BLOCK);
    const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
    const uint8_t stoneBlockId = static_cast<uint8_t>(Blocks::STONE_BLOCK);
    const uint8_t grassPlantId = static_cast<uint8_t>(Blocks::GRASS);
    const uint8_t poppyFlowerId = static_cast<uint8_t>(Blocks::POPPY_FLOWER);
    const uint8_t dandelionFlowerId = static_cast<uint8_t>(Blocks::DANDELION_FLOWER);

    while (y > 5 && level->BoundCheckMap(x, y, z) && level->GetBlockFromMap(x, y, z) == airBlockId) {
        y--;
    }
    
    if (y <= 4) {
        return false;
    }

    y -= 4;

    // Use a flat array on the stack (16*16*8 = 2048 bytes)
    bool grid[2048] = {false};

    int numSpots = (rand() % 4) + 4;
    for (int i = 0; i < numSpots; i++) {
        float radiusX = ((float)rand() / RAND_MAX) * 6.0f + 3.0f;
        float radiusY = ((float)rand() / RAND_MAX) * 4.0f + 2.0f;
        float radiusZ = ((float)rand() / RAND_MAX) * 6.0f + 3.0f;

        float posX = ((float)rand() / RAND_MAX) * (16.0f - radiusX - 2.0f) + 1.0f + radiusX / 2.0f;
        float posY = ((float)rand() / RAND_MAX) * (8.0f - radiusY - 4.0f) + 2.0f + radiusY / 2.0f;
        float posZ = ((float)rand() / RAND_MAX) * (16.0f - radiusZ - 2.0f) + 1.0f + radiusZ / 2.0f;

        float invHRX = 1.0f / (radiusX * 0.5f);
        float invHRY = 1.0f / (radiusY * 0.5f);
        float invHRZ = 1.0f / (radiusZ * 0.5f);

        for (int currX = 1; currX < 15; currX++) {
            for (int currZ = 1; currZ < 15; currZ++) {
                for (int currY = 1; currY < 7; currY++) {
                    float dx = (currX - posX) * invHRX;
                    float dy = (currY - posY) * invHRY;
                    float dz = (currZ - posZ) * invHRZ;
                    float distSq = dx * dx + dy * dy + dz * dz;
                    if (distSq < 1.0f) {
                        grid[((currX) * 16 + (currZ)) * 8 + (currY)] = true;
                    }
                }
            }
        }
    }

    for (int currX = 0; currX < 16; currX++) {
        for (int currZ = 0; currZ < 16; currZ++) {
            for (int currY = 0; currY < 8; currY++) {
                bool isBorder = !grid[((currX) * 16 + (currZ)) * 8 + (currY)] && (
                         (currX < 15 && grid[((currX + 1) * 16 + (currZ)) * 8 + (currY)])
                        || (currX > 0 && grid[((currX - 1) * 16 + (currZ)) * 8 + (currY)])
                        || (currZ < 15 && grid[((currX) * 16 + (currZ + 1)) * 8 + (currY)])
                        || (currZ > 0 && grid[((currX) * 16 + (currZ - 1)) * 8 + (currY)])
                        || (currY < 7 && grid[((currX) * 16 + (currZ)) * 8 + (currY + 1)])
                        || (currY > 0 && grid[((currX) * 16 + (currZ)) * 8 + (currY - 1)]));

                if (isBorder) {
                    if (!level->BoundCheckMap(x + currX, y + currY, z + currZ)) return false;
                    uint8_t blkAt = level->GetBlockFromMap(x + currX, y + currY, z + currZ);
                    
                    if (currY >= 4 && (blkAt == waterBlockId || blkAt == lavaBlockId)) {
                        return false;
                    }

                    bool isSolid = (blkAt != airBlockId && 
                                    blkAt != waterBlockId && 
                                    blkAt != lavaBlockId &&
                                    blkAt != grassPlantId &&
                                    blkAt != poppyFlowerId &&
                                    blkAt != dandelionFlowerId);

                    if (currY < 4 && !isSolid && blkAt != liquidBlockType) {
                        return false;
                    }
                }
            }
        }
    }

    for (int currX = 0; currX < 16; currX++) {
        for (int currZ = 0; currZ < 16; currZ++) {
            for (int currY = 0; currY < 8; currY++) {
                if (grid[((currX) * 16 + (currZ)) * 8 + (currY)]) {
                    placeBlock(level, x + currX, y + currY, z + currZ, currY >= 4 ? airBlockId : liquidBlockType);
                    if (currY < 4) {
                        level->SetLiquidDataToMap(x + currX, y + currY, z + currZ, static_cast<uint8_t>(LiquidLevel::Percent100));
                    }
                }
            }
        }
    }

    for (int currX = 0; currX < 16; currX++) {
        for (int currZ = 0; currZ < 16; currZ++) {
            for (int currY = 4; currY < 8; currY++) {
                if (grid[((currX) * 16 + (currZ)) * 8 + (currY)]) {
                    if (level->BoundCheckMap(x + currX, y + currY - 1, z + currZ)) {
                        uint8_t blkBelowId = level->GetBlockFromMap(x + currX, y + currY - 1, z + currZ);
                        if (blkBelowId == dirtyBlockId && level->GetSunLightFromMap(x + currX, y + currY, z + currZ) > 0) {
                            placeBlock(level, x + currX, y + currY - 1, z + currZ, grassBlockId);
                        }
                    }
                }
            }
        }
    }

    if (liquidBlockType == lavaBlockId) {
        for (int currX = 0; currX < 16; currX++) {
            for (int currZ = 0; currZ < 16; currZ++) {
                for (int currY = 0; currY < 8; currY++) {
                    bool isBorder = !grid[((currX) * 16 + (currZ)) * 8 + (currY)] && (
						(currX < 15 && grid[(((currX + 1) * 16 + (currZ)) * 8 + (currY))])
						|| (currX > 0 && grid[(((currX - 1) * 16 + (currZ)) * 8 + (currY))])
						|| (currZ < 15 && grid[(((currX) * 16 + (currZ + 1)) * 8 + (currY))])
						|| (currZ > 0 && grid[(((currX) * 16 + (currZ - 1)) * 8 + (currY))])
						|| (currY < 7 && grid[(((currX) * 16 + (currZ)) * 8 + (currY + 1))])
						|| (currY > 0 && grid[(((currX) * 16 + (currZ)) * 8 + (currY - 1))]));

                    if (isBorder) {
                        if (level->BoundCheckMap(x + currX, y + currY, z + currZ)) {
                            uint8_t blkAt = level->GetBlockFromMap(x + currX, y + currY, z + currZ);
                            bool isSolid = (blkAt != airBlockId && 
                                            blkAt != waterBlockId && 
                                            blkAt != lavaBlockId &&
                                            blkAt != grassPlantId &&
                                            blkAt != poppyFlowerId &&
                                            blkAt != dandelionFlowerId);

                            if ((currY < 4 || rand() % 2 != 0) && isSolid) {
                                placeBlock(level, x + currX, y + currY, z + currZ, stoneBlockId);
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}
