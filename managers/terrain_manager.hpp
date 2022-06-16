#ifndef _TERRAIN_MANAGER_
#define _TERRAIN_MANAGER_

#include <tamtypes.h>
#include <engine.hpp>
#include <game.hpp>
#include <modules/texture_repository.hpp>
#include <models/mesh.hpp>
#include <models/bounding_box.hpp>
#include <models/math/vector3.hpp>
#include <models/math/ray.hpp>
#include <fastmath.h>
#include "../objects/Block.hpp"
#include "../objects/chunck.hpp"
#include "../objects/player.hpp"
#include "../include/contants.hpp"
#include "../3libs/FastNoiseLite/FastNoiseLite.h"
#include "./block_manager.hpp"

class TerrainManager
{
public:
    TerrainManager();
    ~TerrainManager();
    void init(TextureRepository *t_texRepo);
    void update(Player *t_player, Camera *t_camera, const Pad &t_pad);
    void generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves);
    inline Chunck *getChunck() { return this->chunck; };
    Chunck *getChunck(int offsetX, int offsetY, int offsetZ);
    void updateChunkByPlayerPosition(Player *player);

    Block *targetBlock = NULL;
    void getTargetBlock(const Vector3 &playerPosition, Camera *t_camera);
    void removeBlock();
    void putBlock(u8 blockType);

    TextureRepository *texRepo;
    Engine *engine;

    Vector3 worldSpawnArea;
    Vector3 spawnArea;
    void defineSpawnArea();
    const Vector3 calcSpawOffset(int bias = 0);

    int getNoise(int x, int z);

private:
    Ray ray;
    u8 shouldUpdateChunck = 0;

    Player *t_player;
    Camera *t_camera;

    // TODO: Refactor to region and cache it. See https://minecraft.fandom.com/el/wiki/Region_file_format;
    Chunck *chunck;
    u8 *terrain = new u8[OVERWORLD_SIZE];
    Vector3 minWorldPos;
    Vector3 maxWorldPos;

    Vector3 lastPlayerPosition;
    int blockToRemoveIndex;
    int blockToPlaceIndex;

    BlockManager *blockManager = new BlockManager();

    // Params for noise generation;
    const float frequency = 0.01;
    const float amplitude = 0.65f;
    const float lacunarity = 2.4f;
    const float persistance = .45f;
    const unsigned int seed = rand() % 100000;//5215;
    int octaves = sqrt(OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE);

    FastNoiseLite *noise;
    void initNoise();
    u8 getBlock(int noise, int y);
    float getContinentalness(int x, int z);
    float getErosion(int x, int z);
    float getPeaksAndValleys(int x, int z);
    float getDensity(int x, int y, int z);
    float getTemperature(int x, int z);
    float getHumidity(int x, int z);
    float getHeightScale(int x, int z);

    void buildChunk(int offsetX, int offsetY, int offsetZ);
    int getBlockTypeByPosition(int x, int y, int z);
    unsigned int getIndexByOffset(int x, int y, int z);
    unsigned int getIndexByPosition(Vector3 *pos);
    Vector3 *getPositionByIndex(unsigned int index);
    bool isBlockHidden(int x, int y, int z);
    inline bool isBlockVisible(int x, int y, int z) { return !isBlockHidden(x, y, z); };

    void handlePadControls(const Pad &t_pad);
    Vector3 *normalizeWorldBlockPosition(Vector3 *worldPosition);
};

#endif
