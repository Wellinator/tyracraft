#pragma once

#include <engine.hpp>
#include <tamtypes.h>
#include <time/timer.hpp>
#include <pad/pad.hpp>
#include <renderer/renderer.hpp>
#include "chunck.hpp"
#include "entities/player/player.hpp"
#include "entities/Block.hpp"
#include "managers/items_repository.hpp"
#include "constants.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include <vector>
#include "managers/chunck_manager.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "models/new_game_model.hpp"

using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::Pad;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Vec4;

class World {
 public:
  World(const NewGameOptions& options);
  ~World();

  Renderer* t_renderer;
  TerrainManager* terrainManager;
  BlockManager* blockManager;
  ChunckManager* chunckManager;

  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            SoundManager* t_soundManager);
  void update(Player* t_player, Camera* t_camera, Pad* t_pad,
              const float& deltaTime);
  void render();
  inline const Vec4 getGlobalSpawnArea() const { return this->worldSpawnArea; };
  inline const Vec4 getLocalSpawnArea() const { return this->spawnArea; };
  const std::vector<Block*> getLoadedBlocks();
  void buildInitialPosition();
  inline const int getSeed() { return this->terrainManager->getSeed(); };

 private:
  MinecraftPipeline mcPip;
  StaticPipeline stapip;
  std::vector<Block*> loadedBlocks;
  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  Vec4* lastPlayerPosition = new Vec4();
  u8 framesCounter = 0;
  NewGameOptions worldOptions = NewGameOptions();

  Color dayColor = Color(192.0F, 216.0F, 255.0F);

  int maxLoadPerCall = 1;
  int maxUnloadPerCall = 2;

  std::vector<Chunck*> tempChuncksToLoad;
  std::vector<Chunck*> tempChuncksToUnLoad;
  std::vector<McpipBlock*> overlayData;

  // TODO: move to day/night cycle manager
  Vec4 fixedPoint = CENTER_WORLD_POS;
  float currentAngle = 0.0F;
  float angularSpeed = 0.1F;
  float circleRad = HALF_OVERWORLD_H_DISTANCE * DUBLE_BLOCK_SIZE;
  std::vector<Vec4> lightsPositions = {
      Vec4(0.0F, 0.0F, 0.0F)  // Sun positions
  };
  Vec4 getNewSunPosition();

  void updateChunkByPlayerPosition(Player* player);
  void scheduleChunksNeighbors(Chunck* t_chunck, u8 force_loading = 0);
  void loadScheduledChunks();
  void unloadScheduledChunks();
  void reloadChangedChunk();
  void addChunkToLoadAsync(Chunck* t_chunck);
  void addChunkToUnloadAsync(Chunck* t_chunck);
  void renderBlockDamageOverlay();
  void renderTargetBlockHitbox(Block* targetBlock);
};
