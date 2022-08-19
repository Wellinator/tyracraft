
#include "entities/World.hpp"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"

using Tyra::Color;
using Tyra::M4x4;

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

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad,
                   const float& deltaTime) {
  this->framesCounter++;
  this->updateChunkByPlayerPosition(t_player);
  this->terrainManager->update(t_player, t_camera, t_pad,
                               this->chunckManager->getChuncks(), deltaTime);
  this->chunckManager->update(t_player);
  if (this->framesCounter >= 60) this->framesCounter = 0;
};

void World::render() {
  // TODO: Render only chunck in view frustum
  this->chunckManager->renderer(this->t_renderer, &this->mcPip,
                                this->blockManager);
  this->renderBlockDamageOverlay();
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

  if (this->isLoadingData) {
    if (framesCounter % 25 == 0) this->loadNextChunk();
    return;
  } else if (this->terrainManager->shouldUpdateChunck()) {
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
      this->scheduleChunksNeighbors(currentChunck);
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

void World::scheduleChunksNeighbors(Chunck* t_chunck) {
  Vec4 offset = (*t_chunck->maxCorner + *t_chunck->minCorner) / 2;
  auto chuncks = this->chunckManager->getChuncks();
  for (u16 i = 0; i < chuncks.size(); i++) {
    // Prevent to reload the current chunck more than once
    if (t_chunck->id == chuncks[i]->id) continue;
    Vec4 tempOffset = (*chuncks[i]->maxCorner + *chuncks[i]->minCorner) / 2;
    float distanceToCenterChunck =
        floor(offset.distanceTo(tempOffset) / CHUNCK_SIZE);
    if (distanceToCenterChunck <= DRAW_DISTANCE_IN_CHUNKS) {
      if (!chuncks[i]->isLoaded) tempChuncksToLoad.push_back(chuncks[i]);
    } else if (chuncks[i]->isLoaded) {
      chuncks[i]->clear();
    }
  }

  if (tempChuncksToLoad.size() > 0) this->isLoadingData = 1;
}

void World::loadNextChunk() {
  for (u16 i = 0; i <= tempChuncksToLoad.size(); i++) {
    this->isLoadingData = i < tempChuncksToLoad.size();
    if (isLoadingData) {
      if (tempChuncksToLoad[i]->isLoaded) continue;
      this->terrainManager->buildChunk(tempChuncksToLoad[i]);
    }
    break;
  }
  if (!this->isLoadingData) {
    tempChuncksToLoad.clear();
  }
}

void World::renderBlockDamageOverlay() {
  if (this->terrainManager->isBreakingBLock()) {
    McpipBlock* overlay = this->blockManager->getDamageOverlay(
        this->terrainManager->targetBlock->damage);
    if (overlay != nullptr) {
      if (this->overlayData.size() > 0) {
        // Clear last overlay;
        for (u8 i = 0; i < overlayData.size(); i++) {
          delete overlayData[i]->color;
          delete overlayData[i]->model;
        }
        this->overlayData.clear();
        this->overlayData.shrink_to_fit();
      }

      M4x4 scale = M4x4();
      M4x4 translation = M4x4();

      scale.identity();
      translation.identity();
      scale.scale(BLOCK_SIZE + 0.01f);
      translation.translate(*this->terrainManager->targetBlock->getPosition());

      overlay->model = new M4x4(translation * scale);
      overlay->color = new Color(128.0f, 128.0f, 128.0f, 70.0f);

      this->overlayData.push_back(overlay);
      this->t_renderer->renderer3D.usePipeline(&this->mcPip);
      mcPip.render(overlayData, this->blockManager->getBlocksTexture(), false);
    }
  }
}
