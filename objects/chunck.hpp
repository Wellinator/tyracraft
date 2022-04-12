#ifndef _CHUNCK_
#define _CHUNCK_

#include <vector>
#include <models/math/vector3.hpp>
#include <modules/renderer.hpp>
#include <fastmath.h>
#include "Block.hpp"
#include "player.hpp"
#include "../include/contants.hpp"
#include "../utils.hpp"

class Chunck
{
public:
    Chunck();
    ~Chunck();

    TextureRepository *texRepo;
    std::vector<Block *> blocks;
    Mesh **meshes;
    u16 meshesCount;

    void renderer(Renderer *t_renderer);
    void update(Player *t_player);
    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };
    void add(Block *t_node);
    void clear();
    float getVisibityByPosition(float d);

private:
};

#endif
