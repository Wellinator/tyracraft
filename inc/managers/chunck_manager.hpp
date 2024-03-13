// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include <debug/debug.hpp>
#include "constants.hpp"
#include "entities/chunck.hpp"
#include "entities/player/player.hpp"
#include "managers/block_manager.hpp"
#include <math/vec4.hpp>
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include <math/m4x4.hpp>
#include <vector>
#include <queue>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Vec4;

class ChunckManager {
 public:
  ChunckManager();
  ~ChunckManager();

  inline std::vector<Chunck*>* getChuncks() { return &chuncks; };

  void init(WorldLightModel* worldLightModel, LevelMap* terrain);
  void update(const Plane* frustumPlanes);
  void tick();

  inline u8 isChunkVisible(Chunck* chunk) { return chunk->isVisible(); };

  void renderer(Renderer* t_renderer, StaticPipeline* stapip,
                BlockManager* t_blockManager);
  void rendererOpaque(Renderer* t_renderer, StaticPipeline* stapip,
                      BlockManager* t_blockManager);
  void rendererTransparent(Renderer* t_renderer, StaticPipeline* stapip,
                           BlockManager* t_blockManager);
  void clearAllChunks();

  void enqueueChunksToReloadLight();
  void reloadLightData();
  void reloadLightDataOfAllChunks();

  size_t getChuncksToUpdateLightCount() { return chuncksToUpdateLight.size(); };

  Chunck* getChunkById(const u16& id);
  Chunck* getChunkByBlockOffset(const Vec4& offset);
  Chunck* getChunkByPosition(const Vec4& chunkMinPosition);
  Chunck* getChunkByOffset(const Vec4& chunkMinOffset);
  Chunck* getChunckByWorldPosition(const Vec4& pos);
  Vec4 getChunkPosById(const uint16_t& id);
  void getChunkPosById(const uint16_t& id, Vec4* result);

 private:
  WorldLightModel* worldLightModel;
  LevelMap* terrain;

  std::queue<Chunck*> chuncksToUpdateLight;
  std::vector<Chunck*> chuncks;
  std::vector<Chunck*> visibleChunks;

  LevelMap* map = nullptr;
  void generateChunks();

  void reloadLightDataAsync();
  void clearLightDataQueue() {
    while (!chuncksToUpdateLight.empty()) chuncksToUpdateLight.pop();
  };

  const uint16_t getChunkIdByPosition(const Vec4& chunkMinPosition);
  const uint16_t getChunkIdByOffset(const Vec4& chunkMinOffset);
  u8 isTimeToUpdateLight = 0;
};
