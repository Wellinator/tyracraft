
#include "World.hpp"

void World::init()
{
    this->terrainManager->generateNewTerrain(WORLD_SIZE);
    
    //TODO:
    //Create initial chunk by terrain position offset
    return;
};

void World::update()
{
    //TODO:
    //Update chunck through terrain while player change position
    return;
};

void World::render(Engine *t_engine)
{
    return;
};
