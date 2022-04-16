#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <tamtypes.h>
#include <engine.hpp>
#include <game.hpp>
#include <modules/texture_repository.hpp>
#include <models/mesh.hpp>
#include <models/math/vector3.hpp>
#include <models/math/ray.hpp>
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
    void update(Player *t_player, Camera *t_camera);
    void generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves);
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);


    void updateTargetBlock(Player *t_player, Camera *t_camera);
    void removeBlock(Vector3 *position);
    void putBlock(Vector3 *position, u8 &blockType);

    TextureRepository *texRepo;
    Engine *engine;

private:
    Ray ray;
    u8 shouldUpdateChunck = 0;

    //TODO: Refactor to region and cache it. See https://minecraft.fandom.com/el/wiki/Region_file_format;
    Chunck *chunck;
    u8 *terrain = new u8[OVERWORLD_SIZE];

    Vector3 lastPlayerPosition;
    std::vector<Block *> tempBlocks;
    BlockManager *blockManager = new BlockManager();

    // Params for noise generation;
    const float scale = 10; // 32.0f;
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
    inline bool isBlockVisible(int x, int y, int z) { return !isBlockHidden(x, y, z); };

    void clearTempBlocks();

    int getBlock(int x, int y, int z);
    SimplexNoise *simplex = new SimplexNoise(frequency, amplitude, lacunarity, persistance);
};

#endif
