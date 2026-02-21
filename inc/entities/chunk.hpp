#pragma once

// Forward declaration for visibility graph
#include "managers/visibility_graph.hpp"

#include <vector>
#include <math/vec4.hpp>
#include <renderer/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
#include <functional>
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

enum class ChunkState { 
  Clean,      // Chunk descarregado, sem geometria
  Building,   // Chunk em fila de construção OU sendo construído
  Loaded,     // Chunk construído, pronto para render
  Unloading   // Chunk em fila de descarregamento
};

struct ChunkQuadData {
  // How many blocks this quad is spanning in X, Y and Z axis
  Vec4 span = Vec4(1, 1, 1);
  Vec4 normal = Vec4(0, 0, 0);  // Normal vector of the quad face
  std::array<Vec4, 6> vertices;
  std::array<Vec4, 6> uv;
  std::array<Color, 6> colors;
};

// 16 bytes per vertex (was 48 bytes)
struct CompressedVertex {
  Block::CompressedVec4 pos;  // 8 bytes
  u16 u, v;                   // 4 bytes
  u32 color;                  // 4 bytes
};

class Chunk {
 public:
  Chunk(const Vec4& minOffset, const Vec4& maxOffset, const u16& id);
  ~Chunk();

  u16 id = 0;

  ChunkState state = ChunkState::Clean;
  const bool isLoaded() const { return state == ChunkState::Loaded; }
  const bool isBuilding() const { return state == ChunkState::Building; }
  const bool isUnloading() const { return state == ChunkState::Unloading; }
  const ChunkState getState() const { return state; }

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

  // Pre-computed neighbor pointers for O(1) BFS lookup (populated by ChunkManager::populateNeighborCache())
  // Index = face id (0=TOP, 1=BOTTOM, 2=LEFT, 3=RIGHT, 4=FRONT, 5=BACK); nullptr = world boundary
  Chunk* neighbors[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

  // Visibility graph for cave culling (15-bit face-pair connectivity)
  u16 visibilityGraph = 0;
  bool visibilityGraphDirty = true;
  void rebuildVisibilityGraph();
  bool isConnected(u8 faceA, u8 faceB) const;
  void clearDrawData();
  void clearDrawDataWithoutShrink();
  void invalidateDecompCache();

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

  void setDistanceFromPlayerInChunks(const int distance);

  inline void setCamPosition(Vec4* pos) { this->camPositon.set(*pos); };

  inline u32 getIndexByOffset(int x, int y, int z) {
    return (y * pLevel->map.length * pLevel->map.width) +
           (z * pLevel->map.width) + x;
  }

  const int getLODFromDistance();
  const int getLODFromDistance(const int distance);
  static const int getLODFromDistanceWithHysteresis(const int distance,
                                                    const int currentLOD);

  void updateLOD();
  void compress(const u8 colorTolerance = 10, const float uvTolerance = 0.01f,
                const float normalDotThreshold = 0.99f,
                const bool mergeAcrossUvs = false,
                const bool includeTransparent = true,
                const int maxMergeCount = 0);
  void markDirty();
  inline bool isDirty() { return dirty; }

  // Phase 1: Distance cache management
  inline void markDistanceDirty() { distanceCacheDirty = true; }
  inline float getCachedDistanceSquared() const { return cachedDistanceSquared; }
  inline bool isDistanceCacheDirty() const { return distanceCacheDirty; }
  void updateDistanceCache(const Vec4& playerPos);

  // Callback system for chunk lifecycle events
  using OnLoadedCallback = std::function<void(Chunk*)>;
  void setOnLoadedCallback(OnLoadedCallback cb) { onLoadedCallback = cb; }

  // Fade-in state for smooth chunk transitions (fade-out not needed - chunks unload outside view)
  float fadeAlpha = 0.0f;      // Current fade opacity (0.0 = transparent, 1.0 = opaque)
  bool isFadingIn = false;     // Whether chunk is fading in
  bool isLODRebuild = false;   // True when rebuild is due to LOD change (skip fade reset)
  bool isEmpty = false;        // True when chunk contains only air blocks
  u8 consecutiveOccludedFrames = 0;
  u32 loadedAtTick = 0;  // Tick counter when chunk was loaded (for recently-loaded protection)

  // Configurable fade-in duration (in seconds)
  static constexpr float FADE_IN_DURATION = 0.5f;



 private:
  int randomTickSpeed = DEFAULT_TICK_SPEED;
  void tickRandomBlock();

  bool isCompressed = false;
  bool isUltraCompressed = false;
  bool isMerged = false;
  void buildNormaly();
  void buildMerged();       // LOD 0: no merge
  void buildLOD1();         // LOD 1: merge max 2 faces
  void buildLOD2();         // LOD 2: merge max 3 faces
  void buildUltraCompressed(); // LOD 3: unlimited greedy merge
  void mergeGeometry(const u8 colorTolerance, const float uvTolerance,
                     const float normalDotThreshold, const bool mergeAcrossUvs,
                     const bool includeTransparent, const int maxMergeCount = 0);
  void flushDrawData(Renderer* t_renderer, StaticPipeline* stapip,
                     std::vector<Vec4>* inVertices,
                     std::vector<Color>* inColors, std::vector<Vec4>* inUVs,
                     int offset, int count);
  void sortFacesByNormal(std::vector<Vec4>& verts, std::vector<Vec4>& uvs,
                         std::vector<Color>& cols, int boundaries[7]);
  
  // Compression helpers
  void compressData();
  void decompressData(std::vector<Vec4>* outVertices,
                      std::vector<Color>* outColors, std::vector<Vec4>* outUV,
                      const std::vector<CompressedVertex>& inData);
  
  static inline u32 packColor(const Color& color);
  static inline Color unpackColor(const u32& color);
  static inline void packUV(const Vec4& uv, u16& u, u16& v);
  static inline void unpackUV(Vec4& out, const u16& u, const u16& v);

  std::vector<CompressedVertex> compressedVertices;
  std::vector<CompressedVertex> compressedTransparentVertices;

  // Cached decompressed data for render (avoids per-frame decompression)
  std::vector<Vec4> cachedDecompVertices;
  std::vector<Color> cachedDecompColors;
  std::vector<Vec4> cachedDecompUV;
  std::vector<Vec4> cachedDecompTranspVertices;
  std::vector<Color> cachedDecompTranspColors;
  std::vector<Vec4> cachedDecompTranspUV;
  bool decompCacheValid = false;
  bool decompTranspCacheValid = false;

  Vec4 camPositon = Vec4(0, 0, 0);
  int _distanceFromPlayerInChunks = -1;

  // Phase 1: Distance cache optimization
  float cachedDistanceSquared = -1.0f;  // Cached squared distance to player
  bool distanceCacheDirty = true;        // Cache invalidation flag

  // Refactore the clipped blocks for not using blocks array
  Plane* frustumPlanes = nullptr;

  int _lod = 0;
  int _geometryLod = -1;  // LOD level the geometry was actually built at (-1 = not built)
  bool dirty = false;

  OnLoadedCallback onLoadedCallback;

  // Face-direction group boundaries for back face culling
  // vertices[faceGroupBoundaries[i]..faceGroupBoundaries[i+1]) = group i
  // Groups: 0=TOP(+Y), 1=BOTTOM(-Y), 2=LEFT(+X), 3=RIGHT(-X), 4=FRONT(-Z), 5=BACK(+Z)
  static constexpr int kFaceGroupCount = 6;
  int faceGroupBoundaries[7] = {};
  int transpFaceGroupBoundaries[7] = {};

  std::vector<Vec4> vertices;
  std::vector<Vec4> UV;
  std::vector<Color> colors;

  std::vector<Vec4> transpVertices;
  std::vector<Vec4> transpUV;
  std::vector<Color> transpColors;

  void mergeFaces(std::vector<ChunkQuadData>* outQuadsData,
                  std::vector<Vec4>* inVertices, std::vector<Color>* inColors,
                  std::vector<Vec4>* inUVs, const u8 colorTolerance,
                  const float uvTolerance, const float normalDotThreshold,
                  const bool mergeAcrossUvs, const int maxMergeCount = 0);

  // Helper methods for face merging
  void getQuadBounds(const ChunkQuadData& quad, Vec4& minBounds,
                     Vec4& maxBounds);
  void mergeQuadPair(ChunkQuadData& target, const ChunkQuadData& source);
  void expandQuadGeometry(ChunkQuadData& target, const ChunkQuadData& source,
                          const Vec4& direction, int expansionAxis);
  Vec4 calculateQuadCenter(const ChunkQuadData& quad);
};
