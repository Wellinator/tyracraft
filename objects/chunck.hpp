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

class Chunck
{
public:
    Chunck(BlockManager *t_blockManager);
    ~Chunck();

    //Should be length of bocks types;
    u8 *mesheCounters = new u8[BLOCKS_COUNTER];

    //Should be length of bocks types;
    //Init all chunck's meshes empty;
    std::vector<Mesh *> blocksMeshes[BLOCKS_COUNTER];

    TextureRepository *texRepo;
    Mesh **grassMeshes;

    void renderer(Renderer *t_renderer);
    void update(Player *t_player);
    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };
    void clear();
    float getVisibityByPosition(float d);

    //Block mesh controllers
    void addMeshByBlockType(u8 blockType, Mesh *t_mesh);

private:
    BlockManager *blockManager;
    u8 waitForClear = 0;
};

#endif
