
#include "World.hpp"

World::World(Engine *t_engine)
{
    this->engine = t_engine;
}

World::~World() {}

void World::init()
{
    this->terrainManager->init(engine);
    this->chunck = this->terrainManager->getChunck(0, 0, 0);
    return;
};

void World::update(Player *t_player, Camera *t_camera)
{
    this->terrainManager->updateChunkByPlayerPosition(t_player);
    this->terrainManager->update(t_player, t_camera);
    
    this->chunck->renderer();
};
