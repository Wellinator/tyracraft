
#include "World.hpp"

World::World()
{
}

World::~World() {}

void World::init(TextureRepository *t_texRepo, Player *t_player)
{
    this->terrainManager->init(t_texRepo, t_player);
    this->chunck = this->terrainManager->getChunck( t_player->getPosition().x, 
                                                    t_player->getPosition().y, 
                                                    t_player->getPosition().z);
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
