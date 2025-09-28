// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include <debug/debug.hpp>
#include "constants.hpp"
#include "entities/chunk.hpp"
#include "managers/block_manager.hpp"
#include <math/vec4.hpp>
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include <math/m4x4.hpp>
#include <vector>
#include <queue>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"
#include "singleton.hpp"

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Vec4;

class ChunkManager : public Singleton<ChunkManager> {
 public:
  ChunkManager();
  ~ChunkManager();

  inline std::vector<Chunk*>* getChunks() { return &chunks; };

  void init(WorldLightModel* worldLightModel, Level* Level);
  void update(const Plane* frustumPlanes, Vec4* camPos);
  void tick();

  inline u8 isChunkVisible(Chunk* chunk) { return chunk->isVisible(); };

  void renderer(Renderer* t_renderer, StaticPipeline* stapip);
  void rendererOpaque(Renderer* t_renderer, StaticPipeline* stapip);
  void rendererTransparent(Renderer* t_renderer, StaticPipeline* stapip);
  void clearAllChunks();

  void enqueueChunksToReloadLight();
  void reloadLightData();
  void reloadLightDataOfAllChunks();
  void updateLoadedChunks();

  std::vector<Chunk*>* getLoadedChunks() { return &loadedChunks; };

  size_t getChunksToUpdateLightCount() { return chunksToUpdateLight.size(); };

  Chunk* getChunkById(const u16& id);
  Chunk* getChunkByBlockOffset(const Vec4& offset);
  Chunk* getChunkByPosition(const Vec4& chunkMinPosition);
  Chunk* getChunkByOffset(const Vec4& chunkMinOffset);
  Chunk* getChunkByWorldPosition(const Vec4& pos);
  Vec4 getChunkPosById(const uint16_t& id);
  void getChunkPosById(const uint16_t& id, Vec4* result);

  // TODO: implement to heightmap
  int getHeightAtOffset(const Vec4& offset);
  float getHeightAtPosition(const Vec4& position);

 private:
  WorldLightModel* worldLightModel;
  Level* pLevel;

  std::queue<Chunk*> chunksToUpdateLight;
  std::vector<Chunk*> chunks;
  std::vector<Chunk*> loadedChunks;
  std::vector<Chunk*> visibleChunks;

  void generateChunks();

  void reloadLightDataAsync();
  void clearLightDataQueue() {
    while (!chunksToUpdateLight.empty()) chunksToUpdateLight.pop();
  };

  const uint16_t getChunkIdByPosition(const Vec4& chunkMinPosition);
  const uint16_t getChunkIdByOffset(const Vec4& chunkMinOffset);
  u8 isTimeToUpdateLight = 0;
};
