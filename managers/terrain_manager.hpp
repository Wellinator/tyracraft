#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <tamtypes.h>
#include <engine.hpp>
#include <game.hpp>
#include <modules/texture_repository.hpp>
#include <fastmath.h>
#include "../objects/Block.hpp"
#include "../objects/chunck.hpp"
#include "../objects/player.hpp"
#include "../include/contants.hpp"

class TerrainManager
{
public:
    TerrainManager();
    ~TerrainManager();
    void init(Engine *t_engine);
    void generateNewTerrain(int terrainLength);
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);
    void buildChunk(int offsetX, int offsetY, int offsetZ);
    inline Block *getTerrain() { return terrain; }
    void update();
    Mesh &getMeshByBlockType(int blockType);
    void linkTextureByBlockType(int blockType, const u32 t_meshId);
    void loadBlocks();
    void optimizeTerrain();
    Block *getBlockByIndex(int offsetX, int offsetY, int offsetZ);

    TextureRepository *texRepo;
    Engine *engine;

private:
    int blockIndex;
    Chunck *chunck;
    Block terrain[WORLD_SIZE * WORLD_SIZE * WORLD_SIZE];
    Mesh dirtBlock;
};

#endif