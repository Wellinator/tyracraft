
#include "World.hpp"

World::World(TextureRepository *t_texRepo)
{
    this->t_texRepo = t_texRepo;
}

World::~World() {}

void World::init()
{
    this->terrainManager->init(t_texRepo);
    this->chunck = this->terrainManager->getChunck(0, 0, 0);
    return;
};

void World::update(Player *t_player, Camera *t_camera)
{
    this->terrainManager->updateChunkByPlayerPosition(t_player);
    this->terrainManager->update(t_player, t_camera);
};

void World::render(Renderer *t_renderer)
{
    this->chunck->renderer(t_renderer);
};
