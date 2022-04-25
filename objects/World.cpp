
#include "World.hpp"

World::World()
{
}

World::~World() {}

void World::init(TextureRepository *t_texRepo)
{
    this->terrainManager->init(t_texRepo);
    this->chunck = this->terrainManager->getChunck(0, 0, 0);
    return;
};

void World::update(Player *t_player, Camera *t_camera, const Pad &t_pad)
{
    this->terrainManager->update(t_player, t_camera, t_pad);
};

void World::render(Renderer *t_renderer)
{
    this->chunck->renderer(t_renderer);
};
