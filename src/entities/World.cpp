
#include "entities/World.hpp"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"
#include <tyra>

using Tyra::Color;
using Tyra::M4x4;

World::World(const NewGameOptions& options) {
  this->worldOptions = options;
  this->terrainManager = new TerrainManager();
  this->blockManager = new BlockManager();
  this->chunckManager = new ChunckManager();
}

World::~World() {
  delete lastPlayerPosition;
  delete this->terrainManager;
  delete this->blockManager;
  delete this->chunckManager;
}

void World::init(Renderer* t_renderer, ItemRepository* itemRepository,
                 SoundManager* t_soundManager) {
  TYRA_ASSERT(t_renderer, "t_renderer not initialized");
  TYRA_ASSERT(itemRepository, "itemRepository not initialized");

  // Set color day
  // TODO: refector to day/light cycle;
  t_renderer->core.setClearScreenColor(Color(192.0F, 216.0F, 255.0F));

  this->t_renderer = t_renderer;

  this->mcPip.setRenderer(&t_renderer->core);
  this->stapip.setRenderer(&t_renderer->core);

  this->blockManager->init(t_renderer, &this->mcPip);
  this->chunckManager->init();
  this->terrainManager->init(t_renderer, itemRepository, &this->mcPip,
                             this->blockManager, t_soundManager);

  this->terrainManager->generateNewTerrain(this->worldOptions);
  // Define global and local spawn area
  this->worldSpawnArea.set(this->terrainManager->defineSpawnArea());
  this->spawnArea.set(this->worldSpawnArea);
  this->buildInitialPosition();
};

void World::update(Player* t_player, Camera* t_camera, Pad* t_pad,
                   const float& deltaTime) {
  this->framesCounter++;
  if (this->terrainManager->shouldUpdateChunck()) return reloadChangedChunk();
  this->chunckManager->update(
      this->t_renderer->core.renderer3D.frustumPlanes.getAll());
  this->updateChunkByPlayerPosition(t_player);
  this->terrainManager->update(t_player, t_camera, t_pad,
                               this->chunckManager->getVisibleChunks(),
                               deltaTime);
  if (this->framesCounter >= 60) this->framesCounter = 0;
};

void World::render() {
  this->chunckManager->renderer(this->t_renderer, &this->stapip,
                                this->blockManager);

  if (this->terrainManager->targetBlock) {
    this->renderTargetBlockHitbox(this->terrainManager->targetBlock);
    if (this->terrainManager->isBreakingBLock())
      this->renderBlockDamageOverlay();
  }
};

void World::buildInitialPosition() {
  Chunck* initialChunck =
      this->chunckManager->getChunckByPosition(this->worldSpawnArea);
  if (initialChunck != nullptr) {
    initialChunck->clear();
    this->terrainManager->buildChunk(initialChunck);
    this->scheduleChunksNeighbors(initialChunck, true);
  }
};

const std::vector<Block*> World::getLoadedBlocks() {
  this->loadedBlocks.clear();
  this->loadedBlocks.shrink_to_fit();

  auto visibleChuncks = this->chunckManager->getVisibleChunks();
  for (u16 i = 0; i < visibleChuncks.size(); i++) {
    for (u16 j = 0; j < visibleChuncks[i]->blocks.size(); j++)
      this->loadedBlocks.push_back(visibleChuncks[i]->blocks[j]);
  }

  return this->loadedBlocks;
}

void World::updateChunkByPlayerPosition(Player* t_player) {
  Vec4 currentPlayerPos = *t_player->getPosition();
  if (this->lastPlayerPosition->distanceTo(currentPlayerPos) > CHUNCK_SIZE) {
    this->lastPlayerPosition->set(currentPlayerPos);
    Chunck* currentChunck =
        this->chunckManager->getChunckByPosition(*t_player->getPosition());

    if (currentChunck != nullptr &&
        t_player->currentChunckId != currentChunck->id) {
      t_player->currentChunckId = currentChunck->id;
      this->scheduleChunksNeighbors(currentChunck);
    }
  }

  if (this->framesCounter % 30 == 0) {
    this->unloadScheduledChunks();
    this->loadScheduledChunks();
  }
}

void World::reloadChangedChunk() {
  Chunck* chunckToUpdate = this->chunckManager->getChunckByPosition(
      this->terrainManager->getModifiedPosition());

  if (chunckToUpdate != nullptr) {
    chunckToUpdate->clear();
    this->terrainManager->buildChunk(chunckToUpdate);
    this->terrainManager->setChunckToUpdated();
  }
}

void World::scheduleChunksNeighbors(Chunck* t_chunck, u8 force_loading) {
  Vec4 offset = (*t_chunck->maxOffset + *t_chunck->minOffset) / 2;
  auto chuncks = this->chunckManager->getChuncks();

  for (u16 i = 0; i < chuncks.size(); i++) {
    Vec4 tempOffset = (*chuncks[i]->maxOffset + *chuncks[i]->minOffset) / 2;
    float distanceToCenterChunck =
        floor(offset.distanceTo(tempOffset) / CHUNCK_SIZE) + 1;

    if (distanceToCenterChunck <= DRAW_DISTANCE_IN_CHUNKS) {
      if (force_loading) {
        chuncks[i]->clear();
        this->terrainManager->buildChunk(chuncks[i]);
      } else if (chuncks[i]->state != ChunkState::Loaded &&
                 chuncks[i]->isVisible())
        this->addChunkToLoadAsync(chuncks[i]);
    } else if (!chuncks[i]->isVisible() ||
               chuncks[i]->state != ChunkState::Clean) {
      this->addChunkToUnloadAsync(chuncks[i]);
    }
  }
}

void World::loadScheduledChunks() {
  if (tempChuncksToLoad.size() == 0) return;
  int loadPerCall = 0;

  for (u16 i = 0; i < tempChuncksToLoad.size(); i++) {
    if (tempChuncksToLoad[i]->state == ChunkState::Loaded) continue;
    this->terrainManager->buildChunk(tempChuncksToLoad[i]);

    if (loadPerCall >= this->maxLoadPerCall)
      return;
    else
      loadPerCall++;
  }

  tempChuncksToLoad.clear();
}

void World::unloadScheduledChunks() {
  if (tempChuncksToUnLoad.size() == 0) return;
  int unloadPerCall = 0;
  for (u16 i = 0; i < tempChuncksToUnLoad.size(); i++) {
    if (tempChuncksToUnLoad[i]->state == ChunkState::Clean) continue;
    tempChuncksToUnLoad[i]->clear();

    if (unloadPerCall >= this->maxUnloadPerCall)
      return;
    else
      unloadPerCall++;
  }

  tempChuncksToUnLoad.clear();
}

void World::renderBlockDamageOverlay() {
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

void World::renderTargetBlockHitbox(Block* targetBlock) {
  this->t_renderer->renderer3D.utility.drawBox(*targetBlock->getPosition(),
                                               BLOCK_SIZE, Color(0, 0, 0));
}

void World::addChunkToLoadAsync(Chunck* t_chunck) {
  // Avoid being suplicated;
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  tempChuncksToLoad.push_back(t_chunck);
}

void World::addChunkToUnloadAsync(Chunck* t_chunck) {
  // Avoid being suplicated;
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id) return;

  tempChuncksToUnLoad.push_back(t_chunck);
}
