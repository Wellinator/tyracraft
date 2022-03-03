#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <tamtypes.h>
#include <engine.hpp>
#include <game.hpp>
#include <modules/texture_repository.hpp>
#include <models/mesh.hpp>
#include <fastmath.h>
#include "../objects/Block.hpp"
#include "../objects/Node.hpp"
#include "../objects/chunck.hpp"
#include "../objects/player.hpp"
#include "../include/contants.hpp"
#include "../3libs/SimplexNoise.h"

class TerrainManager
{
public:
    TerrainManager();
    ~TerrainManager();
    void init(Engine *t_engine);
    void update();
    void generateNewTerrain(int terrainLength, int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves);
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);
    inline Block *getTerrain() { return terrain; }

    TextureRepository *texRepo;
    Engine *engine;

private:
    Chunck *chunck;
    Block terrain[WORLD_SIZE * WORLD_SIZE * WORLD_SIZE];
    Vector3 lastPlayerPosition;
    
    //TODO: Refactor to BlockManager entity;
    //Blocks meshes
    Mesh stoneBlock;
    Mesh dirtBlock;
    Mesh grassBlock;

    // Params for noise generation;
    float frequency = .07f;
    float amplitude = 0.5f;
    float lacunarity = 1.99f;
    float persistance = 0.5;
    unsigned int seed = rand() % 10000; // 237;

    void buildChunk(int offsetX, int offsetY, int offsetZ);
    Block *getBlockByIndex(int offsetX, int offsetY, int offsetZ);
    void optimizeTerrain();
    void loadBlocks();
    void linkTextureByBlockType(int blockType, const u32 t_meshId);
    Mesh &getMeshByBlockType(int blockType);
    int getBlock(int x, int y, int z);
    SimplexNoise *simplex = new SimplexNoise(frequency, amplitude, lacunarity, persistance);
};

#endif