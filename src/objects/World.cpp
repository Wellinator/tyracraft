
#include "entities/World.hpp"

World::World() {}

World::~World() {}

void World::init(Renderer* t_renderer, ItemRepository* itemRepository) {
  this->t_renderer = t_renderer;
  this->terrainManager->init(t_renderer, itemRepository);
  this->chunck = this->terrainManager->getChunck(
      floor(this->terrainManager->worldSpawnArea.x / DUBLE_BLOCK_SIZE),
      floor(this->terrainManager->worldSpawnArea.y / DUBLE_BLOCK_SIZE),
      floor(this->terrainManager->worldSpawnArea.z / DUBLE_BLOCK_SIZE));
  return;
};

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad) {
  this->terrainManager->update(t_player, t_camera, t_pad);
};

void World::render() {
  this->chunck->renderer(this->t_renderer);
};
