
#include "World.hpp"

World::World()
{
}

World::~World() {}

void World::init(TextureRepository *t_texRepo, ItemRepository *itemRepository)
{
    this->terrainManager->init(t_texRepo, itemRepository);
    this->chunck = this->terrainManager->getChunck(floor(this->terrainManager->worldSpawnArea.x / DUBLE_BLOCK_SIZE),
                                                   floor(this->terrainManager->worldSpawnArea.y / DUBLE_BLOCK_SIZE),
                                                   floor(this->terrainManager->worldSpawnArea.z / DUBLE_BLOCK_SIZE));
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
