
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
  this->generateChunks();
  this->terrainManager->init(t_renderer, itemRepository, &this->mcPip);
  
  //Define global and local spawn area
  this->worldSpawnArea.set(this->terrainManager->defineSpawnArea());
  this->spawnArea.set(this->worldSpawnArea);

  // TODO: Load inital chuncks from player position
};

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad) {
  this->terrainManager->update(t_player, t_camera, t_pad);
};

void World::render() {
  // TODO: Render only chunck in view frustum
  this->terrainManager->getChunck()->renderer(this->t_renderer, &this->mcPip);
};

void World::generateChunks() {
  u16 tempId = 1;
  for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE;
       x += CHUNCK_SIZE) {
    for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE;
         z += CHUNCK_SIZE) {
      Vec4 tempMin = Vec4(x, OVERWORLD_MIN_HEIGH, z);
      Vec4 tempMax =
          Vec4(x + CHUNCK_SIZE, OVERWORLD_MAX_HEIGH, z + CHUNCK_SIZE);
      Chunck* tempChunck = new Chunck(this->terrainManager->blockManager,
                                      tempMin, tempMax, tempId);
      this->chuncks.push_back(tempChunck);
      tempId++;
    }
  }
};
