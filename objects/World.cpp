
#include "World.hpp"

World::World(Engine *t_engine)
{
    this->engine = t_engine;
}

World::~World() {}

void World::init()
{
    PRINT_LOG("Creating World");
    this->terrainManager->init(engine);
    this->chunck = this->terrainManager->getChunck(0, 0, 0);
    // TODO:
    // Update get chunck by player position

    PRINT_LOG("World created!");
    return;
};

void World::update()
{
    this->terrainManager->update();
    this->chunck->renderer();
    return;
};
