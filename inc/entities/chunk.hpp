#pragma once

#include <vector>
#include <math/vec4.hpp>
#include <renderer/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
#include "entities/Block.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "renderer/3d/bbox/bbox.hpp"
#include <math/m4x4.hpp>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"
#include <array>
#include <time.h>
#include "managers/block/vertex_block_data.hpp"

using Tyra::BBox;
using Tyra::BBoxFace;
using Tyra::Color;
using Tyra::CoreBBoxFrustum;
using Tyra::M4x4;
using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::PipelineDirLightsBag;
using Tyra::PipelineTransformationType;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipLightingBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;
using Tyra::Vec4;

enum class ChunkState { Loaded, Clean };

class Chunk {
 public:
  Chunk(const Vec4& minOffset, const Vec4& maxOffset, const u16& id);
  ~Chunk();

  u16 id = 0;

  ChunkState state = ChunkState::Clean;
  const bool isLoaded() const { return state == ChunkState::Loaded; }

  Vec4 tempLoadingOffset = Vec4();
  Vec4 minOffset = Vec4();
  Vec4 maxOffset = Vec4();
  Vec4 center = Vec4();
  Vec4 scaledMinOffset = Vec4();
  Vec4 scaledMaxOffset = Vec4();
  Vec4 scaledCenterOffset = Vec4();
  BBox* bbox;

  Level* pLevel;
  WorldLightModel* t_worldLightModel;

  clock_t buildingTimeStart;
  double timeToBuild = 0;

  void init(Level* pLevel, WorldLightModel* t_worldLightModel);
  void renderer(Renderer* t_renderer, StaticPipeline* stapip);
  void rendererTransparentData(Renderer* t_renderer, StaticPipeline* stapip);
  void update(const Plane* frustumPlanes);
  void tick();
  void clear();
  void build();
  void rebuild();

  void reloadLightData();
  void clearDrawData();
  void clearDrawDataWithoutShrink();

  u8 containsBlock(Vec4* offset);

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

  inline void setCamPosition(Vec4* pos) { this->camPositon.set(*pos); };

  inline u32 getIndexByOffset(int x, int y, int z) {
    return (y * pLevel->map.length * pLevel->map.width) +
           (z * pLevel->map.width) + x;
  }

  void updateSurroundingBlocks();

 private:
  std::vector<Vec4> vertices;
  std::vector<Color> verticesColors;
  std::vector<Vec4> uvMap;

  // Transparency data
  std::vector<Vec4> verticesWithTransparency;
  std::vector<Color> verticesColorsWithTransparency;
  std::vector<Vec4> uvMapWithTransparency;

  int randomTickSpeed = DEFAULT_TICK_SPEED;
  void tickRandomBlock();

  inline void resetLoadingOffset() { tempLoadingOffset.set(minOffset); };

  Vec4 camPositon = Vec4(0, 0, 0);
  s8 _distanceFromPlayerInChunks = -1;

  // Refactore the clipped blocks for not using blocks array
  std::vector<Block*> surroundingBlocks;
  std::vector<Block*> surroundingTransparentBlocks;
  Plane* frustumPlanes = nullptr;

  void renderSolidPartialBlocks(Renderer* t_renderer, StaticPipeline* stapip);

  void renderTransparentPartialBlocks(Renderer* t_renderer,
                                      StaticPipeline* stapip);

  void renderPartialBlockDrawData(Renderer* t_renderer, u8 hasTransparency,
                                  StaticPipeline* stapip,
                                  std::vector<Vec4>& in_vertex,
                                  std::vector<Vec4>& in_uv,
                                  std::vector<Color>& in_colors);
};
