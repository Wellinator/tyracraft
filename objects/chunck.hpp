#ifndef _CHUNCK_
#define _CHUNCK_

#include <vector>
#include <models/math/vector3.hpp>
#include <modules/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
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
    //Init all chunck's meshes empty;
    std::vector<Block *> blocks;
    std::vector<Mesh *> meshes;

    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };

    void renderer(Renderer *t_renderer);
    void update(Player *t_player);
    void sanitize(Vector3 currentPlayerPos);
    void clear();

    //Block controllers
    void addBlock(Block *t_block);
    
private:
    TextureRepository *texRepo;
    BlockManager *blockManager;

    float getVisibityByPosition(float d);
    void applyFOG(Mesh *t_mesh, const Vector3 &originPosition);
    void highLightTargetBlock(Mesh *t_mesh, u8 &isTarget);
    void updateBlocks(const Vector3 &playerPosition);
};

#endif
