#pragma once

#include <engine.hpp>
#include <tamtypes.h>
#include <time/timer.hpp>
#include <pad/pad.hpp>
#include <renderer/renderer.hpp>
#include "chunck.hpp"
#include "entities/player.hpp"
#include "entities/Block.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/items_repository.hpp"
#include "managers/chunck_manager.hpp"
#include "contants.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include <vector>
#include "managers/block_manager.hpp"

using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::Pad;
using Tyra::Renderer;
using Tyra::Vec4;

class World {
 public:
  World();
  ~World();

  Renderer* t_renderer;
  TerrainManager* terrainManager = new TerrainManager();
  BlockManager* blockManager = new BlockManager();
  ChunckManager* chunckManager = new ChunckManager();

  void init(Renderer* t_renderer, ItemRepository* itemRepository);
  void update(Player* t_player, Camera* t_camera, Pad* t_pad,
              const float& deltaTime);
  void render();
  inline const Vec4 getGlobalSpawnArea() const { return this->worldSpawnArea; };
  inline const Vec4 getLocalSpawnArea() const { return this->spawnArea; };
  const std::vector<Block*> getLoadedBlocks();
  void buildInitialPosition();

 private:
  MinecraftPipeline mcPip;
  std::vector<Block*> loadedBlocks;
  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4* lastPlayerPosition = new Vec4();
  u8 framesCounter = 0;

  std::vector<Chunck*> tempChuncksToLoad;
  std::vector<Chunck*> tempChuncksToUnLoad;
  std::vector<McpipBlock*> overlayData;

  void updateChunkByPlayerPosition(Player* player);
  void scheduleChunksNeighbors(Chunck* t_chunck, u8 force_loading = 0);
  void loadNextChunk();
  void unloadChunckAsync();
  void addChunkToLoadAsync(Chunck* t_chunck);
  void addChunkToUnloadAsync(Chunck* t_chunck);
  void renderBlockDamageOverlay();
};
