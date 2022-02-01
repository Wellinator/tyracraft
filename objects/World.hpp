#ifndef _WORLD_
#define _WORLD_

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/timer.hpp>
#include <modules/pad.hpp>
#include <modules/texture_repository.hpp>
#include "chunck.hpp"

class World
{
public:
    World(Engine *t_engine);
    ~World();

    void init();
    void update();
    void render(Engine *t_engine);

private:
    Chunck *chunck;
};

#endif