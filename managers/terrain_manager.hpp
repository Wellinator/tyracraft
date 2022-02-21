#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <modules/texture_repository.hpp>
#include "block_manager.hpp"
#include "../objects/Block.hpp"
#include "../objects/chunck.hpp"
#include "../objects/player.hpp"
#include "../include/contants.hpp"
#include <engine.hpp>
#include <tamtypes.h>
#include <models/texture.hpp>
#include <models/mesh.hpp>

class TerrainManager
{
public:
    TerrainManager();
    ~TerrainManager();
    void init(TextureRepository *t_repository);
    void generateNewTerrain(int terrainLength);
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);
    void initChunk();
    inline Block *getTerrain() { return terrain; }
    void render(Engine *t_engine);

    TextureRepository *texRepo;
    BlockManager *blockManager;

private:
    int blockIndex;
    Chunck *chunck;
    Block terrain[WORLD_SIZE * WORLD_SIZE * WORLD_SIZE];
};

#endif