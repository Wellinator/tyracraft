#ifndef _WORLD_
#define _WORLD_

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/timer.hpp>
#include <modules/pad.hpp>
#include <modules/texture_repository.hpp>
#include "chunck.hpp"
#include "player.hpp"
#include "../managers/terrain_manager.hpp"
#include "../include/contants.hpp"

class World
{
public:
    World(Engine *t_engine);
    ~World();

    void init();
    void update(Player *t_player, Camera *t_camera);
    Chunck *chunck;

private:
    Engine *engine;
    TerrainManager *terrainManager = new TerrainManager();
};

#endif