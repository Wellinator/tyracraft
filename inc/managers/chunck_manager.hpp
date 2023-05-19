// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include <debug/debug.hpp>
#include "constants.hpp"
#include "entities/chunck.hpp"
#include "entities/player/player.hpp"
#include "managers/block_manager.hpp"
#include <math/vec4.hpp>
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "renderer/3d/bbox/bbox.hpp"
#include <math/m4x4.hpp>
#include <vector>
#include <queue>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Vec4;

class ChunckManager {
 public:
  ChunckManager();
  ~ChunckManager();

  Chunck* getChunckByPosition(const Vec4& position);
  Chunck* getChunckByOffset(const Vec4& offset);
  Chunck* getChunckById(const u16& id);

  std::vector<Chunck*>& getChuncks() { return chuncks; };

  std::vector<Chunck*>& getVisibleChunks();
  inline const u16 getVisibleChunksCounter() { return visibleChunks.size(); };

  void init();
  void update(const Plane* frustumPlanes, const Vec4& currentPlayerPos,
              WorldLightModel* worldLightModel, LevelMap* terrain);
  u8 isChunkVisible(Chunck* chunk);
  void renderer(Renderer* t_renderer, StaticPipeline* stapip,
                BlockManager* t_blockManager);
  void clearAllChunks();
  void sortChunkByPlayerPosition(Vec4* playerPosition);

  void enqueueChunksToReloadLight();

 private:
  std::queue<Chunck*> chuncksToUpdateLight;
  std::vector<Chunck*> chuncks;
  std::vector<Chunck*> visibleChunks;

  LevelMap* map = nullptr;
  void generateChunks();

  void reloadLightData(LevelMap* terrain);
};
