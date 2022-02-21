
#include "World.hpp"

World::World(Engine *t_engine)
{
    this->engine = t_engine;
}

World::~World() {}

void World::init()
{
    PRINT_LOG("Creating World");
    this->terrainManager->init(engine->renderer->getTextureRepository());

    // TODO:
    // Update get chunck by player position

    PRINT_LOG("World created!");
    return;
};

void World::update()
{
    // TODO:
    // Update chunck through terrain while player change position
    this->terrainManager->render(engine);
    return;
};
