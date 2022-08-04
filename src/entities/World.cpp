
#include "entities/World.hpp"
#include "renderer/models/color.hpp"

using Tyra::Color;

World::World() {}

World::~World() {}

void World::init(Renderer* t_renderer, ItemRepository* itemRepository) {
  // Set color day
  // TODO: refector to day/light cycle;
  t_renderer->core.setClearScreenColor(Color(192.0F, 216.0F, 255.0F));
  this->t_renderer = t_renderer;
  this->mcPip.setRenderer(&t_renderer->core);
  this->terrainManager->init(t_renderer, itemRepository, &this->mcPip);
};

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad) {
  this->terrainManager->update(t_player, t_camera, t_pad);
};

void World::render() {
  this->terrainManager->getChunck()->renderer(this->t_renderer, &this->mcPip);
};
