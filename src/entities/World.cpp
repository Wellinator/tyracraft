
#include "entities/World.hpp"

World::World() {}

World::~World() {}

void World::init(Renderer* t_renderer, ItemRepository* itemRepository) {
  this->t_renderer = t_renderer;
  printf("Init MCPipe\n");
  this->mcPip.setRenderer(&t_renderer->core);
  printf("Init Terrain manager\n");
  this->terrainManager->init(t_renderer, itemRepository);
};

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad) {
  this->terrainManager->update(t_player, t_camera, t_pad);
};

void World::render() {
  this->terrainManager->getChunck()->renderer(this->t_renderer, &this->mcPip);
};
