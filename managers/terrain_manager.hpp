#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <modules/texture_repository.hpp>
#include "../objects/Block.hpp"

class TerrainManager
{
public:
    TerrainManager();
    ~TerrainManager();

    Block **generateNewTerrain(int terrainLength);

private:
    int blockIndex;
    Block **terrain = new Block *[WORLD_SIZE * WORLD_SIZE * WORLD_SIZE];
    Block *baseBlobk;
};

#endif