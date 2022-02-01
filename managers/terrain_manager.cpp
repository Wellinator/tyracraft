#include "terrain_manager.hpp"
#include "../objects/Block.hpp"

TerrainManager::TerrainManager()
{
    baseBlobk = new Block();
}

TerrainManager::~TerrainManager()
{
    return;
}

Block **TerrainManager::generateNewTerrain(int terrainLength)
{
    baseBlobk->block_type = 0;

    int blockIndex = 0;
    for (size_t x = 0; x < terrainLength; x++)
    {
        for (size_t y = 0; y < terrainLength; y++)
        {
            for (size_t z = 0; z < terrainLength; z++)
            {
                terrain[blockIndex]->init(
                    baseBlobk->mesh,
                    x * BOCK_SIZE * 2,
                    y * -(BOCK_SIZE * 2),
                    z * -(BOCK_SIZE * 2));
                terrain[blockIndex]->isHidden = false;
                blockIndex++;
            }
        }
    }
    return terrain;
}