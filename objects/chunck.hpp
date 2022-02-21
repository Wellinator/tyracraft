#ifndef _CHUNCK_
#define _CHUNCK_

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/timer.hpp>
#include <modules/pad.hpp>
#include <modules/texture_repository.hpp>
#include <vector>
#include "../camera.hpp"

#include "../include/contants.hpp"
#include "Block.hpp"

class Chunck
{
public:
    Chunck();
    ~Chunck();

    Engine *engine;
    TextureRepository *texRepo;
    void renderer();
    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };
    void pushBlock(Block block);

private:
    std::vector<Block> blocks;
};

#endif
