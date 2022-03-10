#ifndef _CHUNCK_
#define _CHUNCK_

#include <engine.hpp>
#include <vector>
#include "../include/contants.hpp"
#include "Block.hpp"

class Chunck
{
public:
    Chunck(Engine *t_engine);
    ~Chunck();

    Engine *engine;
    TextureRepository *texRepo;
    void renderer();
    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };
    void add(Block *t_node);
    void clear();
    std::vector<Block *> blocks;

private:
};

#endif
