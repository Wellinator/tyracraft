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
    void update();
    void generateNewTerrain(int terrainLength);
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);
    inline Block *getTerrain() { return terrain; }

    TextureRepository *texRepo;
    Engine *engine;

private:
    void buildChunk(int offsetX, int offsetY, int offsetZ);
    Block *getBlockByIndex(int offsetX, int offsetY, int offsetZ);
    void optimizeTerrain();
    void loadBlocks();
    void linkTextureByBlockType(int blockType, const u32 t_meshId);
    Mesh &getMeshByBlockType(int blockType);
    int blockIndex;
    Chunck *chunck;
    Block terrain[WORLD_SIZE * WORLD_SIZE * WORLD_SIZE];
    Mesh dirtBlock;
    Vector3 lastPlayerPosition;
};

#endif