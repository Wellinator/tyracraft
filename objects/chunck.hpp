#ifndef _CHUNCK_
#define _CHUNCK_

#include <vector>
#include <models/math/vector3.hpp>
#include <modules/renderer.hpp>
#include <fastmath.h>
#include "Block.hpp"
#include "player.hpp"
#include "../include/contants.hpp"
#include "../managers/block_manager.hpp"
#include "../utils.hpp"

/**
 * Struct containing world block position and terrain index
 * Chunk cache.
 */
struct ChunckCache
{
    ChunckCache(u16 index, Vector3 *t_worldPosition)
    {
        terrainIndex = index;
        worldPosition = t_worldPosition;
    }

    ~ChunckCache()
    {
        delete worldPosition;
    }

    u16 terrainIndex;
    Vector3 *worldPosition;
};

class Chunck
{
public:
    Chunck(BlockManager *t_blockManager);
    ~Chunck();

    //Should be length of bocks types;
    //Init all chunck's meshes empty;
    std::vector<Mesh *> blocksMeshes[BLOCKS_COUNTER];

    //Cache
    std::vector<ChunckCache *> chunckCache[CHUNCK_SIZE*3];

    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };

    void renderer(Renderer *t_renderer);
    void update(Player *t_player);
    void clear();

    //Block mesh controllers
    void addMeshByBlockType(u8 blockType, Mesh *t_mesh);

    //Cache control
    void addToCache(u16 index, Vector3 *t_worldPosition);

private:
    TextureRepository *texRepo;
    BlockManager *blockManager;

    float getVisibityByPosition(float d);
};

#endif
