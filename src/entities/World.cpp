
#include "entities/World.hpp"
#include "renderer/models/color.hpp"
#include "math/m4x4.hpp"
#include <tyra>

// From CrossCraft
#include <stdio.h>
#include "entities/World.hpp"
#include <queue>

using Tyra::Color;
using Tyra::M4x4;

World::World(const NewGameOptions& options) {
  this->worldOptions = options;
  this->terrainManager = new TerrainManager();
  this->blockManager = new BlockManager();
  this->chunckManager = new ChunckManager();
  CrossCraft_World_Init();
}

World::~World() {
  delete lastPlayerPosition;
  delete this->terrainManager;
  delete this->blockManager;
  delete this->chunckManager;

  CrossCraft_World_Deinit();
}

void World::init(Renderer* t_renderer, ItemRepository* itemRepository,
                 SoundManager* t_soundManager) {
  TYRA_ASSERT(t_renderer, "t_renderer not initialized");
  TYRA_ASSERT(itemRepository, "itemRepository not initialized");

  this->t_renderer = t_renderer;

  this->mcPip.setRenderer(&t_renderer->core);
  this->stapip.setRenderer(&t_renderer->core);

  this->blockManager->init(t_renderer, &this->mcPip);
  this->chunckManager->init();
  this->terrainManager->init(t_renderer, itemRepository, &this->mcPip,
                             this->blockManager, t_soundManager);

  this->terrainManager->terrain = CrossCraft_World_GetMapPtr();

  CrossCraft_World_Create_Map(WORLD_SIZE_SMALL);

  if (worldOptions.makeFlat) {
    CrossCraft_World_GenerateMap(WorldType::WORLD_TYPE_FLAT);
  } else {
    CrossCraft_World_GenerateMap(WorldType::WORLD_TYPE_ORIGINAL);
  }

  // Define global and local spawn area
  this->worldSpawnArea.set(this->terrainManager->defineSpawnArea());
  this->spawnArea.set(this->worldSpawnArea);
  this->buildInitialPosition();
  this->setIntialTime();
};

void World::update(Player* t_player, const Vec4& camLookPos,
                   const Vec4& camPosition) {
  this->framesCounter++;

  dayNightCycleManager.update();
  updateLightModel();

  if (this->terrainManager->shouldUpdateChunck()) {
    if (this->terrainManager->hasRemovedABlock())
      reloadChangedChunkByRemovedBlock();
    else
      reloadChangedChunkByPutedBlock();
  }

  this->chunckManager->update(
      this->t_renderer->core.renderer3D.frustumPlanes.getAll(),
      *t_player->getPosition(), &worldLightModel);
  this->updateChunkByPlayerPosition(t_player);
  this->terrainManager->update(camLookPos, camPosition,
                               this->chunckManager->getVisibleChunks());

  this->framesCounter = this->framesCounter % 60;
};

void World::render() {
  this->t_renderer->core.setClearScreenColor(
      dayNightCycleManager.getSkyColor());

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
  if (lastPlayerPosition->distanceTo(currentPlayerPos) > CHUNCK_SIZE) {
    lastPlayerPosition->set(currentPlayerPos);
    Chunck* currentChunck =
        chunckManager->getChunckByPosition(currentPlayerPos);

    if (currentChunck && t_player->currentChunckId != currentChunck->id) {
      t_player->currentChunckId = currentChunck->id;
      scheduleChunksNeighbors(currentChunck, currentPlayerPos);
    }
  }

  if (framesCounter % 3 == 0) {
    unloadScheduledChunks();
    loadScheduledChunks();
  }
}

void World::reloadChangedChunkByPutedBlock() {
  Chunck* chunckToUpdate = this->chunckManager->getChunckByPosition(
      this->terrainManager->getModifiedPosition());

  if (this->terrainManager->targetBlock &&
      this->terrainManager->targetBlock->isAtChunkBorder) {
    updateNeighBorsChunksByModdedBlock(this->terrainManager->targetBlock);
  }

  if (chunckToUpdate != nullptr) {
    chunckToUpdate->clear();
    this->terrainManager->buildChunk(chunckToUpdate);
    this->terrainManager->setChunckToUpdated();
  }
}

void World::reloadChangedChunkByRemovedBlock() {
  Block* removedBlock = this->terrainManager->removedBlock;
  if (removedBlock->isAtChunkBorder)
    updateNeighBorsChunksByModdedBlock(removedBlock);

  Chunck* chunckToUpdate =
      this->chunckManager->getChunckById(removedBlock->chunkId);
  chunckToUpdate->clear();
  this->terrainManager->buildChunk(chunckToUpdate);
  this->terrainManager->removedBlock = nullptr;
  this->terrainManager->setChunckToUpdated();
}

void World::updateNeighBorsChunksByModdedBlock(Block* changedBlock) {
  // Front
  Chunck* frontChunk = this->chunckManager->getChunckByOffset(
      Vec4(changedBlock->offset.x, changedBlock->offset.y,
           changedBlock->offset.z + 1));

  // Back
  Chunck* backChunk = this->chunckManager->getChunckByOffset(
      Vec4(changedBlock->offset.x, changedBlock->offset.y,
           changedBlock->offset.z - 1));

  // Right
  Chunck* rightChunk = this->chunckManager->getChunckByOffset(
      Vec4(changedBlock->offset.x + 1, changedBlock->offset.y,
           changedBlock->offset.z));
  // Left
  Chunck* leftChunk = this->chunckManager->getChunckByOffset(
      Vec4(changedBlock->offset.x - 1, changedBlock->offset.y,
           changedBlock->offset.z));

  if (frontChunk && frontChunk->id != changedBlock->chunkId) {
    frontChunk->clear();
    this->terrainManager->buildChunk(frontChunk);
  } else if (backChunk && backChunk->id != changedBlock->chunkId) {
    backChunk->clear();
    this->terrainManager->buildChunk(backChunk);
  }

  if (rightChunk && rightChunk->id != changedBlock->chunkId) {
    rightChunk->clear();
    this->terrainManager->buildChunk(rightChunk);
  } else if (leftChunk && leftChunk->id != changedBlock->chunkId) {
    leftChunk->clear();
    this->terrainManager->buildChunk(leftChunk);
  }
}

void World::scheduleChunksNeighbors(Chunck* t_chunck,
                                    const Vec4 currentPlayerPos,
                                    u8 force_loading) {
  auto chuncks = chunckManager->getChuncks();
  for (u16 i = 0; i < chuncks.size(); i++) {
    float distance =
        floor(t_chunck->center->distanceTo(*chuncks[i]->center) / CHUNCK_SIZE) +
        1;

    if (distance > DRAW_DISTANCE_IN_CHUNKS) {
      if (chuncks[i]->state != ChunkState::Clean)
        addChunkToUnloadAsync(chuncks[i]);
    } else {
      if (force_loading) {
        chuncks[i]->clear();
        terrainManager->buildChunkAsync(chuncks[i]);
      } else if (chuncks[i]->state != ChunkState::Loaded) {
        addChunkToLoadAsync(chuncks[i]);
      }
    }
  }

  if (tempChuncksToLoad.size()) sortChunksToLoad(currentPlayerPos);
}

void World::sortChunksToLoad(const Vec4& currentPlayerPos) {
  std::sort(tempChuncksToLoad.begin(), tempChuncksToLoad.end(),
            [currentPlayerPos](const Chunck* a, const Chunck* b) {
              const float distanceA =
                  (*a->center * DUBLE_BLOCK_SIZE).distanceTo(currentPlayerPos);
              const float distanceB =
                  (*b->center * DUBLE_BLOCK_SIZE).distanceTo(currentPlayerPos);
              return distanceA < distanceB;
            });
}

void World::loadScheduledChunks() {
  if (tempChuncksToLoad.size() > 0) {
    if (tempChuncksToLoad[0]->state != ChunkState::Loaded)
      return this->terrainManager->buildChunkAsync(tempChuncksToLoad[0]);
    tempChuncksToLoad.erase(tempChuncksToLoad.begin());
  };
}

void World::unloadScheduledChunks() {
  if (tempChuncksToUnLoad.size() > 0) {
    tempChuncksToUnLoad[0]->clear();
    tempChuncksToUnLoad.erase(tempChuncksToUnLoad.begin());
  }
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
    scale.scale(BLOCK_SIZE + 0.015f);
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
    if (tempChuncksToUnLoad[i]->id == t_chunck->id)
      tempChuncksToUnLoad.erase(tempChuncksToUnLoad.begin() + i);

  tempChuncksToLoad.push_back(t_chunck);
}

void World::addChunkToUnloadAsync(Chunck* t_chunck) {
  // Avoid being suplicated;
  for (size_t i = 0; i < tempChuncksToUnLoad.size(); i++)
    if (tempChuncksToUnLoad[i]->id == t_chunck->id) return;

  // Avoid unload and load the same chunk at the same time
  for (size_t i = 0; i < tempChuncksToLoad.size(); i++)
    if (tempChuncksToLoad[i]->id == t_chunck->id)
      tempChuncksToLoad.erase(tempChuncksToLoad.begin() + i);

  tempChuncksToUnLoad.push_back(t_chunck);
}

void World::updateLightModel() {
  worldLightModel.lightsPositions = lightsPositions.data();
  worldLightModel.lightIntensity = dayNightCycleManager.getLightIntensity();
  worldLightModel.sunPosition.set(dayNightCycleManager.getSunPosition());
  worldLightModel.moonPosition.set(dayNightCycleManager.getMoonPosition());
  worldLightModel.ambientLightIntensity =
      dayNightCycleManager.getAmbientLightIntesity();
}

// From CrossCraft
struct LightNode {
  uint16_t x, y, z;
  LightNode(uint16_t lx, uint16_t ly, uint16_t lz, uint16_t l)
      : x(lx), y(ly), z(lz), val(l) {}
  uint16_t val;
};

std::queue<LightNode> lightBfsQueue;
std::queue<LightNode> lightRemovalBfsQueue;

std::queue<LightNode> sunlightBfsQueue;
std::queue<LightNode> sunlightRemovalBfsQueue;

auto encodeID(uint16_t x, uint16_t z) -> uint32_t {
  uint16_t nx = x / 16;
  uint16_t ny = z / 16;
  uint32_t id = nx << 16 | (ny & 0xFFFF);
  return id;
}

void checkAddID(uint32_t* updateIDs, uint32_t id) {
  // Check that the ID is not already existing
  for (int i = 0; i < 10; i++) {
    if (id == updateIDs[i]) return;
  }

  // Find a slot to insert.
  for (int i = 0; i < 10; i++) {
    if (updateIDs[i] == 0xFFFFFFFF) {
      updateIDs[i] = id;
      return;
    }
  }
}

void updateID(uint16_t x, uint16_t z, uint32_t* updateIDs) {
  checkAddID(updateIDs, encodeID(x, z));
}

void propagate(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
               uint32_t* updateIDs) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  updateID(x, z, updateIDs);

  auto blk = GetBlockFromMap(map, x, y, z);

  if ((blk == 0 || blk == 20 || blk == 18 || (blk >= 8 && blk <= 11) ||
       (blk >= 37 && blk <= 40)) &&
      GetLightFromMap(map, x, y, z) + 2 <= lightLevel) {
    if (blk == 18 || (blk >= 8 && blk <= 11) || (blk >= 37 && blk <= 40)) {
      lightLevel -= 2;
    }
    SetLightInMap(map, x, y, z, lightLevel - 1);
    lightBfsQueue.emplace(x, y, z, 0);
  }
}

void propagate(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  if (GetBlockFromMap(map, x, y, z) == 0 &&
      GetLightFromMap(map, x, y, z) + 2 <= lightLevel) {
    SetLightInMap(map, x, y, z, lightLevel - 1);
    sunlightBfsQueue.emplace(x, y, z, 0);
  }
}

void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel,
                     uint32_t* updateIDs) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  auto neighborLevel = GetLightFromMap(map, x, y, z);

  updateID(x, z, updateIDs);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    SetLightInMap(map, x, y, z, 0);
    lightRemovalBfsQueue.emplace(x, y, z, neighborLevel);
  } else if (neighborLevel >= lightLevel) {
    lightBfsQueue.emplace(x, y, z, 0);
  }
}

void propagateRemove(uint16_t x, uint16_t y, uint16_t z, uint16_t lightLevel) {
  auto map = CrossCraft_World_GetMapPtr();
  if (!BoundCheckMap(map, x, y, z)) return;

  auto neighborLevel = GetLightFromMap(map, x, y, z);

  if (neighborLevel != 0 && neighborLevel < lightLevel) {
    SetLightInMap(map, x, y, z, 0);
    sunlightRemovalBfsQueue.emplace(x, y, z, neighborLevel);
  } else if (neighborLevel >= lightLevel) {
    sunlightBfsQueue.emplace(x, y, z, 0);
  }
}

void updateRemove(uint32_t* updateIDs) {
  while (!lightRemovalBfsQueue.empty()) {
    auto node = lightRemovalBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel = node.val;
    lightRemovalBfsQueue.pop();

    propagateRemove(nx + 1, ny, nz, lightLevel, updateIDs);
    propagateRemove(nx - 1, ny, nz, lightLevel, updateIDs);
    propagateRemove(nx, ny + 1, nz, lightLevel, updateIDs);
    propagateRemove(nx, ny - 1, nz, lightLevel, updateIDs);
    propagateRemove(nx, ny, nz + 1, lightLevel, updateIDs);
    propagateRemove(nx, ny, nz - 1, lightLevel, updateIDs);
  }
}

void updateRemove() {
  while (!lightRemovalBfsQueue.empty()) {
    auto node = lightRemovalBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel = node.val;
    lightRemovalBfsQueue.pop();

    propagateRemove(nx + 1, ny, nz, lightLevel);
    propagateRemove(nx - 1, ny, nz, lightLevel);
    propagateRemove(nx, ny + 1, nz, lightLevel);
    propagateRemove(nx, ny - 1, nz, lightLevel);
    propagateRemove(nx, ny, nz + 1, lightLevel);
    propagateRemove(nx, ny, nz - 1, lightLevel);
  }
}

void updateSpread(uint32_t* updateIDs) {
  while (!lightBfsQueue.empty()) {
    auto node = lightBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel =
        GetLightFromMap(CrossCraft_World_GetMapPtr(), nx, ny, nz);
    lightBfsQueue.pop();

    propagate(nx + 1, ny, nz, lightLevel, updateIDs);
    propagate(nx - 1, ny, nz, lightLevel, updateIDs);
    propagate(nx, ny + 1, nz, lightLevel, updateIDs);
    propagate(nx, ny - 1, nz, lightLevel, updateIDs);
    propagate(nx, ny, nz + 1, lightLevel, updateIDs);
    propagate(nx, ny, nz - 1, lightLevel, updateIDs);
  }
}

void updateSpread() {
  while (!lightBfsQueue.empty()) {
    auto node = lightBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    uint8_t lightLevel =
        GetLightFromMap(CrossCraft_World_GetMapPtr(), nx, ny, nz);
    lightBfsQueue.pop();

    propagate(nx + 1, ny, nz, lightLevel);
    propagate(nx - 1, ny, nz, lightLevel);
    propagate(nx, ny + 1, nz, lightLevel);
    propagate(nx, ny - 1, nz, lightLevel);
    propagate(nx, ny, nz + 1, lightLevel);
    propagate(nx, ny, nz - 1, lightLevel);
  }
}

void updateSunlight() {
  while (!sunlightBfsQueue.empty()) {
    auto node = sunlightBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    int8_t lightLevel = node.val - 1;
    sunlightBfsQueue.pop();
    if (lightLevel <= 0) continue;

    propagate(nx + 1, ny, nz, lightLevel);
    propagate(nx - 1, ny, nz, lightLevel);
    propagate(nx, ny + 1, nz, lightLevel);
    propagate(nx, ny - 1, nz, lightLevel);
    propagate(nx, ny, nz + 1, lightLevel);
    propagate(nx, ny, nz - 1, lightLevel);
  }
}

void updateSunlightRemove() {
  while (!sunlightRemovalBfsQueue.empty()) {
    auto node = sunlightRemovalBfsQueue.front();

    uint16_t nx = node.x;
    uint16_t ny = node.y;
    uint16_t nz = node.z;
    int8_t lightLevel = node.val;
    sunlightRemovalBfsQueue.pop();
    if (lightLevel <= 0) continue;

    propagateRemove(nx + 1, ny, nz, lightLevel);
    propagateRemove(nx - 1, ny, nz, lightLevel);
    propagateRemove(nx, ny + 1, nz, lightLevel);
    propagateRemove(nx, ny - 1, nz, lightLevel);
    propagateRemove(nx, ny, nz + 1, lightLevel);
    propagateRemove(nx, ny, nz - 1, lightLevel);
  }
}

void CrossCraft_World_AddLight(uint16_t x, uint16_t y, uint16_t z,
                               uint16_t light, uint32_t* updateIDs) {
  SetLightInMap(CrossCraft_World_GetMapPtr(), x, y, z, light);
  updateID(x, z, updateIDs);
  lightBfsQueue.emplace(x, y, z, light);

  updateSpread(updateIDs);
}

void CrossCraft_World_RemoveLight(uint16_t x, uint16_t y, uint16_t z,
                                  uint16_t light, uint32_t* updateIDs) {
  auto map = CrossCraft_World_GetMapPtr();

  auto val = GetLightFromMap(map, x, y, z);
  lightRemovalBfsQueue.emplace(x, y, z, val);

  SetLightInMap(map, x, y, z, light);
  updateID(x, z, updateIDs);

  updateRemove(updateIDs);
  updateSpread(updateIDs);
}

void singleCheck(uint16_t x, uint16_t y, uint16_t z) {
  auto map = CrossCraft_World_GetMapPtr();

  if (y == 0) return;

  auto lv = 15;
  for (int y2 = map->height - 1; y2 >= 0; y2--) {
    auto blk = GetBlockFromMap(map, x, y2, z);

    if (blk == 18 || (blk >= 37 && blk <= 40) || (blk >= 8 && blk <= 11)) {
      lv -= 2;
    } else if (blk != 0 && blk != 20) {
      lv = 0;
    }

    auto lv2 = GetLightFromMap(map, x, y2, z);

    if (lv2 < lv) {
      SetLightInMap(map, x, y2, z, lv);
      sunlightRemovalBfsQueue.emplace(x, y2, z, 0);
    } else {
      SetLightInMap(map, x, y2, z, lv);
      sunlightBfsQueue.emplace(x, y2, z, lv);
    }
  }
}

bool CrossCraft_World_CheckSunLight(uint16_t x, uint16_t y, uint16_t z) {
  singleCheck(x, y, z);
  singleCheck(x + 1, y, z);
  singleCheck(x - 1, y, z);
  singleCheck(x, y, z + 1);
  singleCheck(x, y, z - 1);

  updateSunlightRemove();
  updateSunlight();

  return true;
}

void CrossCraft_World_PropagateSunLight(uint32_t tick) {
  auto map = CrossCraft_World_GetMapPtr();

  for (int x = 0; x < map->length; x++) {
    for (int z = 0; z < map->width; z++) {
      auto lv = 4;
      if (tick >= 0 && tick <= 12000) {
        lv = 15;
      }

      for (int y = map->height - 1; y >= 0; y--) {
        auto blk = GetBlockFromMap(map, x, y, z);

        if (blk == 18 || (blk >= 37 && blk <= 40) || (blk >= 8 && blk <= 11)) {
          lv -= 2;
        } else if (blk != 0 && blk != 20) {
          break;
        }

        if (lv < 0) break;

        SetLightInMap(map, x, y, z, lv);
        sunlightBfsQueue.emplace(x, y, z, lv);
      }
    }
  }

  updateSunlight();
}

void CrossCraft_World_Init() {
  TYRA_LOG("Generating base level template");
  srand(time(NULL));

  CrossCraft_WorldGenerator_Init(rand());

  LevelMap map = {.width = 256,
                  .length = 256,
                  .height = 64,

                  .spawnX = 128,
                  .spawnY = 59,
                  .spawnZ = 128,

                  .blocks = NULL,
                  .data = NULL};
  level.map = map;

  TYRA_LOG("Generated base level template");
}

void CrossCraft_World_Deinit() {
  TYRA_LOG("Destroying the world");

  if (level.map.blocks) free(level.map.blocks);

  if (level.map.data) free(level.map.data);

  // if (level.entities.entities) free(level.entities.entities);

  // if (level.tileEntities.entities) free(level.tileEntities.entities);

  TYRA_LOG("World freed");
}

void CrossCraft_World_Create_Map(uint8_t size) {
  switch (size) {
    case WORLD_SIZE_SMALL: {
      level.map.length = 128;
      level.map.width = 128;
      level.map.height = 64;
      break;
    }
    case WORLD_SIZE_HUGE: {
      level.map.length = 256;
      level.map.width = 256;
      level.map.height = 64;
      break;
    }
    case WORLD_SIZE_NORMAL:
    default: {
      level.map.length = 192;
      level.map.width = 192;
      level.map.height = 64;
      break;
    }
  }

  level.map.spawnX = level.map.length / 2;
  level.map.spawnY = level.map.height / 2;
  level.map.spawnZ = level.map.width / 2;

  uint32_t blockCount = level.map.length * level.map.height * level.map.width;
  level.map.blocks = new uint8_t[blockCount];
  level.map.data = new uint8_t[blockCount];
}

// #include <nbt.h>

/**
 * @brief Tries to load a world
 * @returns If the world was loaded
 */
// bool CrossCraft_World_TryLoad(uint8_t slot, const char* prefix) {
//     char buf[256] = {0};

//     strcpy(buf, prefix);
//     strcat(buf, "save.ccc");

//     FILE* fptr = fopen(buf, "r");
//     if(fptr) {
//         slot = 5;
//         // TODO: Load and convert

//         fclose(fptr);
//         return true;
//     }

//     if(slot >= 5) {
//         return false;
//     }
//     sprintf(buf, "%slevel%d.mclevel", prefix, slot);

//     TYRA_LOG("Attempting load.");
//     TYRA_LOG(buf);

//     nbt_node* root = nbt_parse_path(buf);

//     if(root == NULL) {
//         CC_Internal_Log_Message(CC_LOG_WARN, "Could not load!");
//         return false;
//     }

//     nbt_node* abt = nbt_find_by_name(root, "About");

//     if(abt != NULL) {
//         level.about.name = nbt_find_by_name(abt, "Name")->payload.tag_string;
//         level.about.author = nbt_find_by_name(abt,
//         "Author")->payload.tag_string; level.about.createdOn =
//         nbt_find_by_name(abt, "CreatedOn")->payload.tag_long;

//         CC_Internal_Log_Message(CC_LOG_TRACE, level.about.name);
//         CC_Internal_Log_Message(CC_LOG_TRACE, level.about.author);
//     }

//     nbt_node *map = nbt_find_by_name(root, "Map");
//     if(map != NULL) {
//         level.map.width = nbt_find_by_name(map, "Width")->payload.tag_short;
//         level.map.length = nbt_find_by_name(map,
//         "Length")->payload.tag_short; level.map.height =
//         nbt_find_by_name(map, "Height")->payload.tag_short;

//         nbt_node* spawn = nbt_find_by_name(map, "Spawn");

//         level.map.spawnX = nbt_list_item(spawn, 0)->payload.tag_short;
//         level.map.spawnY = nbt_list_item(spawn, 1)->payload.tag_short;
//         level.map.spawnZ = nbt_list_item(spawn, 2)->payload.tag_short;

//         uint32_t blockCount = level.map.length * level.map.height *
//         level.map.width; level.map.blocks = malloc(blockCount);
//         level.map.data = malloc(blockCount);

//         struct nbt_byte_array byteArray = nbt_find_by_name(map,
//         "Blocks")->payload.tag_byte_array; memcpy(level.map.blocks,
//         byteArray.data, blockCount); byteArray = nbt_find_by_name(map,
//         "Data")->payload.tag_byte_array; memcpy(level.map.data,
//         byteArray.data, blockCount);
//     }

//     nbt_free(root);

//     return true;
// }

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void CrossCraft_World_GenerateMap(WorldType worldType) {
  switch (worldType) {
    case WORLD_TYPE_ORIGINAL:
      CrossCraft_WorldGenerator_Generate_Original(&level.map);
      break;
    case WORLD_TYPE_FLAT:
      CrossCraft_WorldGenerator_Generate_Flat(&level.map);
      break;
    case WORLD_TYPE_ISLAND:
      CrossCraft_WorldGenerator_Generate_Island(&level.map);
      break;
    case WORLD_TYPE_WOODS:
      CrossCraft_WorldGenerator_Generate_Woods(&level.map);
      break;
    case WORLD_TYPE_FLOATING:
      CrossCraft_WorldGenerator_Generate_Floating(&level.map);
      break;
  }
}

/**
 * @brief Spawn the player into the world
 */
void CrossCraft_World_Spawn() {}

LevelMap* CrossCraft_World_GetMapPtr() { return &level.map; }