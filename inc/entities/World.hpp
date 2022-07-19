#ifndef _WORLD_
#define _WORLD_

#include <engine.hpp>
#include <tamtypes.h>
#include <time/timer.hpp>
#include <pad/pad.hpp>
#include <renderer/renderer.hpp>
#include "chunck.hpp"
#include "entities/player.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/items_repository.hpp"
#include "contants.hpp"

class World
{
public:
    World();
    ~World();

    void init(TextureRepository *t_texRepo, ItemRepository *itemRepository);
    void update(Player *t_player, Camera *t_camera, const Pad &t_pad);
    void render(Renderer *t_renderer);
    Chunck *chunck;
    TerrainManager *terrainManager = new TerrainManager();
};

#endif