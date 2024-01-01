#pragma once

#include <vector>
#include <math/vec4.hpp>
#include <renderer/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
#include "entities/Block.hpp"
#include "constants.hpp"
#include "managers/block_manager.hpp"
#include "utils.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "renderer/3d/bbox/bbox.hpp"
#include <math/m4x4.hpp>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"
#include <array>

using Tyra::BBox;
using Tyra::BBoxFace;
using Tyra::Color;
using Tyra::CoreBBoxFrustum;
using Tyra::M4x4;
using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::PipelineDirLightsBag;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipLightingBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;
using Tyra::Vec4;

enum class ChunkState { PreLoaded, Loaded, Loading, Clean };

class Chunck {
 public:
  Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id);
  ~Chunck();

  u16 id = 0;

  ChunkState state = ChunkState::Clean;

  std::vector<Block*> blocks;
  Vec4 tempLoadingOffset = Vec4();
  Vec4 minOffset = Vec4();
  Vec4 maxOffset = Vec4();
  Vec4 scaledMinOffset = Vec4();
  Vec4 scaledMaxOffset = Vec4();
  Vec4 center = Vec4();
  BBox* bbox;

  // Neighborhoods references;
  Chunck* frontNeighbor = nullptr;
  Chunck* backNeighbor = nullptr;
  Chunck* leftNeighbor = nullptr;
  Chunck* rightNeighbor = nullptr;
  Chunck* topNeighbor = nullptr;
  Chunck* bottomNeighbor = nullptr;

  int visibleFacesCount = 0;
  u16 blocksCount = 0;

  LevelMap* t_terrain;
  WorldLightModel* t_worldLightModel;

  void init(LevelMap* t_terrain, WorldLightModel* t_worldLightModel);
  void renderer(Renderer* t_renderer, StaticPipeline* stapip,
                BlockManager* t_blockManager);
  void update(const Plane* frustumPlanes);
  void clear();

  void loadDrawData();
  void loadDrawDataWithoutSorting();
  void reloadLightData();
  void clearDrawData();
  inline const u8 isDrawDataLoaded() { return _isDrawDataLoaded; };

  CoreBBoxFrustum frustumCheck = CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  void updateFrustumCheck(const Plane* frustumPlanes);
  inline const u8 isVisible() {
    return this->frustumCheck != Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  }

  inline s8 getDistanceFromPlayerInChunks() {
    return this->_distanceFromPlayerInChunks;
  };
  inline void setDistanceFromPlayerInChunks(const s8 distante) {
    this->_distanceFromPlayerInChunks = distante;
  };

  // Block controllers
  inline void addBlock(Block* t_block) {
    blocks.emplace_back(t_block);
    visibleFacesCount += t_block->visibleFacesCount;
  };

  inline void preAllocateMemory() {
    blocks.reserve(CHUNCK_LENGTH);
    _isPreAllocated = true;
  };

  inline void freeUnusedMemory() {
    blocks.shrink_to_fit();
    _isPreAllocated = false;
  };

  inline u8 isPreAllocated() { return _isPreAllocated; };

  Block* getBlockByPosition(const Vec4* pos);
  Block* getBlockByOffset(const Vec4* offset);

 private:
  std::vector<Vec4> vertices;
  std::vector<Color> verticesColors;
  std::vector<Vec4> uvMap;
  void sortBlockByTransparency();

  inline void resetLoadingOffset() { tempLoadingOffset.set(minOffset); };

  u8 _isDrawDataLoaded = false;

  s8 _distanceFromPlayerInChunks = -1;
  bool _isPreAllocated = false;
};
