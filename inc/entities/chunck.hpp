#ifndef _CHUNCK_
#define _CHUNCK_

#include <vector>
#include <math/vec4.hpp>
#include <renderer/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
#include "entities/Block.hpp"
#include "entities/player.hpp"
#include "contants.hpp"
#include "managers/block_manager.hpp"
#include "utils.hpp"


class Chunck
{
public:
    Chunck(BlockManager *t_blockManager);
    ~Chunck();

    //Should be length of bocks types;
    //Init all chunck's meshes empty;
    std::vector<Block *> blocks;
    // std::vector<Mesh *> meshes;

    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };

    void renderer(Renderer *t_renderer);
    void update(Player *t_player);
    void clear();

    //Block controllers
    void addBlock(Block *t_block);
    
private:
    TextureRepository *texRepo;
    BlockManager *blockManager;

    float getVisibityByPosition(float d);
    void applyFOG(Mesh *t_mesh, const Vec4 &originPosition);
    void highLightTargetBlock(Mesh *t_mesh, u8 &isTarget);
    void updateBlocks(const Vec4 &playerPosition);
};

#endif
