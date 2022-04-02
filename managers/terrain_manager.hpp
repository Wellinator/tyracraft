#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <tamtypes.h>
#include <engine.hpp>
#include <game.hpp>
#include <modules/texture_repository.hpp>
#include <models/mesh.hpp>
#include <models/math/vector3.hpp>
#include <fastmath.h>
#include "../objects/Block.hpp"
#include "../objects/chunck.hpp"
#include "../objects/player.hpp"
#include "../include/contants.hpp"
#include "../3libs/SimplexNoise.h"
#include "./block_manager.hpp"

class TerrainManager
{
public:
    TerrainManager();
    ~TerrainManager();
    void init(Engine *t_engine);
    void update();
    void generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves);
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);

    TextureRepository *texRepo;
    Engine *engine;
private:
    Chunck *chunck;
    u64 *terrain = new u64[OVERWORLD_SIZE];
    Vector3 lastPlayerPosition;
    std::vector<Block *> tempBlocks;
    BlockManager *blockManager = new BlockManager();

    // Params for noise generation;
    const float scale =  10;//32.0f;
    const float frequency = 0.007;
    const float amplitude = 0.5f;
    const float lacunarity = 2.4f;
    const float persistance = .45f;
    const unsigned int seed = rand() % 10000; // 237;

    void buildChunk(int offsetX, int offsetY, int offsetZ);
    int getBlockTypeByPosition(int x, int y, int z);
    unsigned int getIndexByPosition(int x, int y, int z);
    Vector3 *getPositionByIndex(unsigned int index);
    bool isBlockHidden(int x, int y, int z);
    inline bool isBlockVisible(int x, int y, int z){return !isBlockHidden(x, y, z);};
    
    void clearTempBlocks();
    
    int getBlock(int x, int y, int z);
    SimplexNoise *simplex = new SimplexNoise(frequency, amplitude, lacunarity, persistance);
};

#endif
