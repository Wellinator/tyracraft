
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
  this->blockManager->init(t_renderer, &this->mcPip);
  this->chunckManager->init();
  this->terrainManager->init(t_renderer, itemRepository, &this->mcPip,
                             this->blockManager);

  // Define global and local spawn area
  this->worldSpawnArea.set(this->terrainManager->defineSpawnArea());
  this->spawnArea.set(this->worldSpawnArea);
  this->buildInitialPosition();
};

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad) {
  this->updateChunkByPlayerPosition(t_player);
  this->terrainManager->update(t_player, t_camera, t_pad,
                               this->chunckManager->getChuncks());
  this->chunckManager->update(t_player);
};

void World::render() {
  // TODO: Render only chunck in view frustum
  this->chunckManager->renderer(this->t_renderer, &this->mcPip,
                                this->blockManager);
};

void World::buildInitialPosition() {
  Chunck* initialChunck =
      this->chunckManager->getChunckByPosition(this->worldSpawnArea);
  this->terrainManager->buildChunk(initialChunck);
  this->buildChunksNeighbors(initialChunck);
};

const std::vector<Block*> World::getLoadedBlocks() {
  this->loadedBlocks.clear();
  this->loadedBlocks.shrink_to_fit();

  auto chuncks = this->chunckManager->getChuncks();
  for (u16 i = 0; i < chuncks.size(); i++) {
    if (chuncks[i]->isLoaded) {
      for (u16 j = 0; j < chuncks[i]->blocks.size(); j++)
        this->loadedBlocks.push_back(chuncks[i]->blocks[j]);
    }
  }

  return this->loadedBlocks;
}

void World::updateChunkByPlayerPosition(Player* t_player) {
  Vec4 currentPlayerPos = *t_player->getPosition();
  if (this->terrainManager->shouldUpdateChunck()) {
    Vec4 changedPosition = *this->terrainManager->targetBlock->getPosition();
    Chunck* chunckToUpdate =
        this->chunckManager->getChunckByPosition(changedPosition);
    this->terrainManager->buildChunk(chunckToUpdate);
    this->terrainManager->setChunckToUpdated();
  } else if (this->lastPlayerPosition->distanceTo(currentPlayerPos) >
             CHUNCK_SIZE) {
    this->lastPlayerPosition->set(currentPlayerPos);
    Chunck* currentChunck =
        this->chunckManager->getChunckByPosition(*t_player->getPosition());
    if (t_player->currentChunckId != currentChunck->id) {
      t_player->currentChunckId = currentChunck->id;
      this->terrainManager->buildChunk(currentChunck);
      this->buildChunksNeighbors(currentChunck);
    }
  }
}

void World::buildChunksNeighbors(Chunck* t_chunck) {
  Vec4 offset = (*t_chunck->maxCorner + *t_chunck->minCorner) / 2;
  auto chuncks = this->chunckManager->getChuncks();
  for (u16 i = 0; i < chuncks.size(); i++) {
    // Prevent to reload the current chunck more than once
    if (t_chunck->id == chuncks[i]->id) continue;

    Vec4 tempOffset = (*chuncks[i]->maxCorner + *chuncks[i]->minCorner) / 2;
    float distanceToCenterChunck =
        floor(offset.distanceTo(tempOffset) / CHUNCK_SIZE);
    if (distanceToCenterChunck <= DRAW_DISTANCE_IN_CHUNKS) {
      if (!chuncks[i]->isLoaded) this->terrainManager->buildChunk(chuncks[i]);
    } else if (chuncks[i]->isLoaded) {
      chuncks[i]->clear();
    }
  }
}
