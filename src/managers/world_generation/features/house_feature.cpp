#include "managers/world_generation/features/house_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool HouseFeature::place(Level* level, int x, int y, int z) {
    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);
    const uint8_t waterBlockId = static_cast<uint8_t>(Blocks::WATER_BLOCK);
    const uint8_t lavaBlockId = static_cast<uint8_t>(Blocks::LAVA_BLOCK);
    const uint8_t voidBlockId = static_cast<uint8_t>(Blocks::VOID);
    const uint8_t stoneBrickId = static_cast<uint8_t>(Blocks::STONE_BRICK_BLOCK);
    const uint8_t mossyStoneBrickId = static_cast<uint8_t>(Blocks::MOSSY_STONE_BRICKS_BLOCK);
    const uint8_t oakPlanksId = static_cast<uint8_t>(Blocks::OAK_PLANKS_BLOCK);
    const uint8_t glassBlockId = static_cast<uint8_t>(Blocks::GLASS_BLOCK);
    const uint8_t torchBlockId = static_cast<uint8_t>(Blocks::TORCH);

    // Traverse down until a solid block is found (skip air, water, lava, void)
    while (y > 0) {
        if (!level->BoundCheckMap(x, y - 1, z)) return false;
        
        uint8_t blockBelowId = level->GetBlockFromMap(x, y - 1, z);
        if (blockBelowId != airBlockId && 
            blockBelowId != waterBlockId &&
            blockBelowId != voidBlockId && 
            blockBelowId != lavaBlockId) {
            break;
        }
        y--;
    }
    
    int width = (rand() % 7) + 7;
    int height = 4 + (rand() % 3) / 2;
    int depth = (rand() % 7) + 7;

    int originX = x - width / 2;
    int originY = y;
    int originZ = z - depth / 2;

    int doorSide = rand() % 4;
    if (doorSide < 2) depth += 2;
    else width += 2;

    // Check if space logic allows the house
    for (int currX = originX; currX < originX + width; currX++) {
        for (int currZ = originZ; currZ < originZ + depth; currZ++) {
            if (!level->BoundCheckMap(currX, y - 1, currZ)) return false;
            
            uint8_t m = level->GetBlockFromMap(currX, y - 1, currZ);
            if (m == airBlockId || m == waterBlockId || m == lavaBlockId) {
                return false; // must be relatively solid ground under walls
            }

            bool isDoorZone = false;
            if (doorSide == 0 && currX < originX + 2) isDoorZone = true;
            if (doorSide == 1 && currX > originX + width - 1 - 2) isDoorZone = true;
            if (doorSide == 2 && currZ < originZ + 2) isDoorZone = true;
            if (doorSide == 3 && currZ > originZ + depth - 1 - 2) isDoorZone = true;

            uint8_t t = level->GetBlockFromMap(currX, y, currZ);
            if (isDoorZone) {
                if (t != airBlockId) return false;
            } else {
                if (t == stoneBrickId || t == mossyStoneBrickId) {
                    return false;
                }
            }
        }
    }

    if (doorSide == 0) {
        originX++;
        width--;
    } else if (doorSide == 1) {
        width--;
    } else if (doorSide == 2) {
        originZ++;
        depth--;
    } else if (doorSide == 3) {
        depth--;
    }

    int innerX0 = originX;
    int innerX1 = originX + width - 1;
    int innerZ0 = originZ;
    int innerZ1 = originZ + depth - 1;
    if (doorSide >= 2) {
        innerX0++;
        innerX1--;
    } else {
        innerZ0++;
        innerZ1--;
    }

    // Build the walls and roof
    for (int currX = originX; currX < originX + width; currX++) {
        for (int currZ = originZ; currZ < originZ + depth; currZ++) {
            int originalHeight = height;

            int dist1 = currZ - originZ;
            int dist2 = (originZ + depth - 1) - currZ;
            if (doorSide < 2) {
                dist1 = currX - originX;
                dist2 = (originX + width - 1) - currX;
            }

            if (dist2 < dist1) dist1 = dist2;
            height += dist1;

            for (int currY = originY - 1; currY < originY + height; currY++) {
                if (!level->BoundCheckMap(currX, currY, currZ)) break;
                
                int materialId = -1;
                if (currY == originY + height - 1) {
                    materialId = static_cast<int>(oakPlanksId);
                } else if (currX >= innerX0 && currX <= innerX1 && currZ >= innerZ0 && currZ <= innerZ1) {
                    materialId = static_cast<int>(airBlockId);
                    if (currY == originY - 1 || currY == originY + height - 1 || currX == innerX0 || currZ == innerZ0 || currX == innerX1 || currZ == innerZ1) {
                        if (currY <= originY + (rand() % 3)) materialId = static_cast<int>(mossyStoneBrickId);
                        else materialId = static_cast<int>(stoneBrickId);
                    }
                }

                if (materialId >= 0) {
                    placeBlock(level, currX, currY, currZ, static_cast<uint8_t>(materialId));
                }
            }
            height = originalHeight;
        }
    }

    // Creating door entrance
    {
        int doorX = originX + (rand() % (width - 4)) + 2;
        int doorZ = originZ + (rand() % (depth - 4)) + 2;
        if (doorSide == 0) doorX = originX;
        if (doorSide == 1) doorX = originX + width - 1;
        if (doorSide == 2) doorZ = originZ;
        if (doorSide == 3) doorZ = originZ + depth - 1;
        
        placeBlock(level, doorX, originY, doorZ, airBlockId);
        placeBlock(level, doorX, originY + 1, doorZ, airBlockId);
    }

    // Add windows
    for (int i = 0; i < (width * 2 + depth * 2) * 3; i++) {
        int winX = originX + (rand() % (width - 4)) + 2;
        int winZ = originZ + (rand() % (depth - 4)) + 2;
        int side = rand() % 4;

        if (side == 0) winX = innerX0;
        if (side == 1) winX = innerX1;
        if (side == 2) winZ = innerZ0;
        if (side == 3) winZ = innerZ1;

        if (!level->BoundCheckMap(winX, originY + 1, winZ)) continue;
        uint8_t blkAt = level->GetBlockFromMap(winX, originY + 1, winZ);
        
        if (blkAt != airBlockId && blkAt != waterBlockId && blkAt != lavaBlockId) { 
            
            int solidNeighborCount = 0;
            auto checkSolid = [&](int cx, int cy, int cz) {
                if (!level->BoundCheckMap(cx, cy, cz)) return false;
                uint8_t b = level->GetBlockFromMap(cx, cy, cz);
                return b != airBlockId && b != waterBlockId && b != lavaBlockId && b != glassBlockId;
            };

            if (checkSolid(winX - 1, originY + 1, winZ) && checkSolid(winX + 1, originY + 1, winZ)) solidNeighborCount++;
            if (checkSolid(winX, originY + 1, winZ - 1) && checkSolid(winX, originY + 1, winZ + 1)) solidNeighborCount++;
            
            if (solidNeighborCount == 1) {
                placeBlock(level, winX, originY + 1, winZ, glassBlockId);
            }
        }
    }

    // Add interior torches
    int innerWidth = innerX1 - innerX0;
    int innerDepth = innerZ1 - innerZ0;
    if (innerWidth > 2 && innerDepth > 2) {
        int totalIterations = (innerWidth * 2 + innerDepth * 2);
        for (int i = 0; i < totalIterations; i++) {
            int torchX = innerX0 + 1 + (rand() % (innerWidth - 1));
            int torchZ = innerZ0 + 1 + (rand() % (innerDepth - 1));
            int torchY = originY + 2;

            if (!level->BoundCheckMap(torchX, torchY, torchZ)) continue;

            if (level->GetBlockFromMap(torchX, torchY, torchZ) == airBlockId) {
                auto checkSolid = [&](int cx, int cy, int cz) {
                    if (!level->BoundCheckMap(cx, cy, cz)) return false;
                    uint8_t b = level->GetBlockFromMap(cx, cy, cz);
                    return b != airBlockId && b != waterBlockId && b != lavaBlockId && 
                           b != glassBlockId && b != torchBlockId && b != voidBlockId;
                };

                bool hasWestWall = checkSolid(torchX - 1, torchY, torchZ);
                bool hasEastWall = checkSolid(torchX + 1, torchY, torchZ);
                bool hasNorthWall = checkSolid(torchX, torchY, torchZ - 1);
                bool hasSouthWall = checkSolid(torchX, torchY, torchZ + 1);

                if (hasWestWall || hasEastWall || hasNorthWall || hasSouthWall) {
                    if (rand() % 10 < 4) continue;

                    placeBlock(level, torchX, torchY, torchZ, torchBlockId);
                    
                    BlockOrientation orient = BlockOrientation::Top;
                    if (hasWestWall) orient = BlockOrientation::West; 
                    else if (hasEastWall) orient = BlockOrientation::East; 
                    else if (hasNorthWall) orient = BlockOrientation::North; 
                    else if (hasSouthWall) orient = BlockOrientation::South; 
                    
                    level->SetTorchOrientationDataToMap(torchX, torchY, torchZ, orient);
                }
            }
        }
    }

    return true;
}
