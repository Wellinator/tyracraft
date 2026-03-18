#include "managers/world_generation/features/tree_feature.hpp"
#include <constants.hpp>
#include <stdlib.h>

bool TreeFeature::isSpaceForTree(Level* pLevel, int x, int y, int z, int treeHeight) {
    const uint8_t grassBlockId = static_cast<uint8_t>(Blocks::GRASS_BLOCK);
    const uint8_t airBlockId = static_cast<uint8_t>(Blocks::AIR_BLOCK);

    if (!pLevel->BoundCheckMap(x, y - 1, z) ||
        pLevel->GetBlockFromMap(x, y - 1, z) != grassBlockId) {
        return false;
    }

    for (int currY = y + 1; currY < y + treeHeight; currY++) {
        if (!pLevel->BoundCheckMap(x, currY, z) ||
            pLevel->GetBlockFromMap(x, currY, z) != airBlockId) {
            return false;
        }
    }

    for (int currX = x - 2; currX <= x + 2; currX++) {
        for (int currY = y + treeHeight; currY < y + treeHeight + 3; currY++) {
            for (int currZ = z - 2; currZ <= z + 2; currZ++) {
                if (!pLevel->BoundCheckMap(currX, currY, currZ) ||
                    pLevel->GetBlockFromMap(currX, currY, currZ) != airBlockId) {
                    return false;
                }
            }
        }
    }

    return true;
}

void TreeFeature::growTree(Level* pLevel, int x, int y, int z, int treeHeight) {
    int maxHeight = y + treeHeight;
    int currY = maxHeight;

    for (; currY >= y; currY--) {
        if (currY == maxHeight) {
            placeBlock(pLevel, x - 1, currY, z, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z, leavesBlockType);
            placeBlock(pLevel, x, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x, currY, z + 1, leavesBlockType);
            placeBlock(pLevel, x, currY, z, leavesBlockType);
        } else if (currY == maxHeight - 1) {
            placeBlock(pLevel, x - 1, currY, z, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z, leavesBlockType);
            placeBlock(pLevel, x, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x, currY, z + 1, leavesBlockType);

            if (rand() % 2 == 0) placeBlock(pLevel, x - 1, currY, z - 1, leavesBlockType);
            if (rand() % 2 == 0) placeBlock(pLevel, x - 1, currY, z + 1, leavesBlockType);
            if (rand() % 2 == 0) placeBlock(pLevel, x + 1, currY, z - 1, leavesBlockType);
            if (rand() % 2 == 0) placeBlock(pLevel, x + 1, currY, z + 1, leavesBlockType);

            placeBlock(pLevel, x, currY, z, logBlockType);
        } else if (currY == maxHeight - 2 || currY == maxHeight - 3) {
            placeBlock(pLevel, x - 1, currY, z, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z, leavesBlockType);
            placeBlock(pLevel, x, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x, currY, z + 1, leavesBlockType);

            placeBlock(pLevel, x - 1, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x - 1, currY, z + 1, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z + 1, leavesBlockType);

            placeBlock(pLevel, x - 2, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x - 2, currY, z, leavesBlockType);
            placeBlock(pLevel, x - 2, currY, z + 1, leavesBlockType);

            placeBlock(pLevel, x + 2, currY, z - 1, leavesBlockType);
            placeBlock(pLevel, x + 2, currY, z, leavesBlockType);
            placeBlock(pLevel, x + 2, currY, z + 1, leavesBlockType);

            placeBlock(pLevel, x - 1, currY, z - 2, leavesBlockType);
            placeBlock(pLevel, x, currY, z - 2, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z - 2, leavesBlockType);

            placeBlock(pLevel, x - 1, currY, z + 2, leavesBlockType);
            placeBlock(pLevel, x, currY, z + 2, leavesBlockType);
            placeBlock(pLevel, x + 1, currY, z + 2, leavesBlockType);

            if (rand() % 2 == 0) placeBlock(pLevel, x - 2, currY, z - 2, leavesBlockType);
            if (rand() % 2 == 0) placeBlock(pLevel, x + 2, currY, z - 2, leavesBlockType);
            if (rand() % 2 == 0) placeBlock(pLevel, x - 2, currY, z + 2, leavesBlockType);
            if (rand() % 2 == 0) placeBlock(pLevel, x + 2, currY, z + 2, leavesBlockType);

            placeBlock(pLevel, x, currY, z, logBlockType);
        } else {
            placeBlock(pLevel, x, currY, z, logBlockType);
        }
    }
}

bool TreeFeature::place(Level* level, int x, int y, int z) {
    int treeHeight = rand() % 3 + 4;
    if (isSpaceForTree(level, x, y, z, treeHeight)) {
        growTree(level, x, y, z, treeHeight);
        return true;
    }
    return false;
}
