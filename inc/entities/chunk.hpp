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

struct ChunkQuadData {
  // How many blocks this quad is spanning in X, Y and Z axis
  Vec4 span = Vec4(1, 1, 1);
  Vec4 normal = Vec4(0, 0, 0);  // Normal vector of the quad face
  std::array<Vec4, 6> vertices;
  std::array<Vec4, 6> uv;
  std::array<Color, 6> colors;
};

class Chunk {
 public:
  Chunk(const Vec4& minOffset, const Vec4& maxOffset, const u16& id);
  ~Chunk();

  u16 id = 0;

  ChunkState state = ChunkState::Clean;
  const bool isLoaded() const { return state == ChunkState::Loaded; }

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

  bool hasDrawData();
  void reloadLightData();
  void clearDrawData();
  void clearDrawDataWithoutShrink();

  u8 containsBlock(Vec4* offset);

  CoreBBoxFrustum frustumCheck = CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  void updateFrustumCheck(const Plane* frustumPlanes);
  inline const u8 isVisible() {
    return this->frustumCheck != Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  }

  inline const bool isPartiallyVisible() {
    return this->frustumCheck == Tyra::CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM;
  }

  inline int getDistanceFromPlayerInChunks() {
    return this->_distanceFromPlayerInChunks;
  };

  inline void setDistanceFromPlayerInChunks(const int distante) {
    this->_distanceFromPlayerInChunks = distante;
  };

  inline void setCamPosition(Vec4* pos) { this->camPositon.set(*pos); };

  inline u32 getIndexByOffset(int x, int y, int z) {
    return (y * pLevel->map.length * pLevel->map.width) +
           (z * pLevel->map.width) + x;
  }

  const int getLODFromDistance();
  const int getLODFromDistance(const int distance);

  void onLodChanged();
  void compress(const bool staticBackFaceCulling = false);
  void optimize();

 private:
  int randomTickSpeed = DEFAULT_TICK_SPEED;
  void tickRandomBlock();

  bool isCompressed = false;
  void buildNormaly();
  void buildCompressed();
  void flushDrawData(Renderer* t_renderer, StaticPipeline* stapip,
                     std::vector<Vec4>* inVertices,
                     std::vector<Color>* inColors, std::vector<Vec4>* inUVs,
                     int limit = -1);

  Vec4 camPositon = Vec4(0, 0, 0);
  int _distanceFromPlayerInChunks = -1;

  // Refactore the clipped blocks for not using blocks array
  Plane* frustumPlanes = nullptr;

  int _lod = 0;
  bool dirty = false;
  void markDirty();

  bool isDrawDataOptimized = false;
  void invalidateOptimization();
  void applyBackFaceCulling(int* targetLimit, std::vector<Vec4>* pVertex,
                            std::vector<Vec4>* pUV,
                            std::vector<Color>* pColors);

  int vertexCutLimit = -1;
  std::vector<Vec4> vertices;
  std::vector<Vec4> UV;
  std::vector<Color> colors;

  int transpVertexCutLimit = -1;
  std::vector<Vec4> transpVertices;
  std::vector<Vec4> transpUV;
  std::vector<Color> transpColors;

  void mergeFaces(std::vector<ChunkQuadData>* outQuadsData,
                  std::vector<Vec4>* inVertices, std::vector<Color>* inColors,
                  std::vector<Vec4>* inUVs);

  // Helper methods for face merging
  void getQuadBounds(const ChunkQuadData& quad, Vec4& minBounds,
                     Vec4& maxBounds);
  void mergeQuadPair(ChunkQuadData& target, const ChunkQuadData& source);
  void expandQuadGeometry(ChunkQuadData& target, const ChunkQuadData& source,
                          const Vec4& direction, int expansionAxis);
  Vec4 calculateQuadCenter(const ChunkQuadData& quad);
};
