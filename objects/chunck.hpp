#ifndef _CHUNCK_
#define _CHUNCK_

#include <engine.hpp>
#include <vector>
#include <models/math/vector3.hpp>
#include <fastmath.h>
#include "../include/contants.hpp"
#include "../utils.hpp"
#include "Block.hpp"
#include "player.hpp"

class Chunck
{
public:
    Chunck(Engine *t_engine);
    ~Chunck();

    Engine *engine;
    TextureRepository *texRepo;
    std::vector<Block *> blocks;
    Mesh **meshes;
    u16 meshesCount = 10;

    void renderer();
    void update(Player *t_player);
    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };
    void add(Block *t_node);
    void clear();
    float getVisibityByPosition(float d);

private:
};

#endif
