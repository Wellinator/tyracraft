#pragma once

// Forward declaration for visibility graph
#include "managers/visibility_graph.hpp"
#include "managers/mesh/binary_greedy_mesher.hpp"

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

/**
 * Phases used by the incremental build pipeline (beginBuild / buildStep).
 * Each phase does a bounded unit of work so the frame budget is respected.
 *
 *  Idle ──[beginBuild]──▶ AirCheck ──▶ MeshGen (BGM, one face-dir per step)
 *                              │               │
 *                         all-air           Done ──▶ VisGraph ──▶ Finalize
 */
enum class BuildPhase : u8 {
  Idle       = 0,  // No incremental build in progress
  AirCheck   = 1,  // Detecting whether the chunk is entirely air
  MeshGen    = 2,  // BGM meshing: one FaceDir per step (6 total)
  VisGraph   = 3,  // Rebuild BFS visibility graph
  Finalize   = 4,  // Transition to Loaded, fire callbacks
};

struct ChunkQuadData {
  // How many blocks this quad is spanning in X, Y and Z axis
  Vec4 span = Vec4(1, 1, 1);
  Vec4 normal = Vec4(0, 0, 0);  // Normal vector of the quad face
  std::array<Vec4, 6> vertices;
  std::array<Vec4, 6> uv;
  std::array<Color, 6> colors;
};

/** One group of merged quads sharing the same atlas tile.
 *  Used for per-tile RegionRepeat rendering in LOD > 0. */
struct TileGroup {
  static constexpr u8 LEGACY_TILE = 255;  // Sentinel: use default wrap (no RegionRepeat)

  u32 start;  // Vertex start index in the flat array
  u32 count;  // Number of vertices (multiple of 6)
  u8  col;    // Atlas column (0-15), or LEGACY_TILE for non-BGM blocks
  u8  row;    // Atlas row (0-15), or LEGACY_TILE for non-BGM blocks
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

  // -----------------------------------------------------------------------
  // Synchronous build (used for force-load, block placement, initial spawn)
  // -----------------------------------------------------------------------
  void build();
  void rebuild();

  // -----------------------------------------------------------------------
  // Incremental build pipeline — distributes work across multiple frames.
  //
  //   beginBuild()  — initialise phases; call once when chunk is dequeued.
  //   buildStep()   — advance exactly one phase; returns true when complete.
  //   cancelBuild() — abort a build-in-progress and reset to Clean state.
  //
  // Typical caller loop (in World::processIdleWork):
  //   chunk->beginBuild();
  //   while (!chunk->buildStep()) { /* check time budget */ }
  // -----------------------------------------------------------------------
  void beginBuild();
  bool buildStep();
  void cancelBuild();

  /** True while an incremental build is active (not Idle). */
  inline bool isBuildingIncrementally() const {
    return buildPhase != BuildPhase::Idle;
  }
  inline BuildPhase getBuildPhase() const { return buildPhase; }

  bool hasDrawData();
  void reloadLightData();
  void reloadLightColorsOnly();  // Fast path: only regenerate colors for day/night

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
  bool isEmpty = false;        // True when chunk contains only air blocks
  u8 consecutiveOccludedFrames = 0;
  u32 loadedAtTick = 0;  // Tick counter when chunk was loaded (for recently-loaded protection)

  // Configurable fade-in duration (in seconds)
  static constexpr float FADE_IN_DURATION = 0.5f;



 private:
  int randomTickSpeed = DEFAULT_TICK_SPEED;
  void tickRandomBlock();

  // -----------------------------------------------------------------------
  // Incremental build state (used by beginBuild / buildStep)
  // -----------------------------------------------------------------------
  BuildPhase buildPhase    = BuildPhase::Idle;
  u8         meshGenFaceDir = 0;  // Current FaceDir being processed (0-6; 6=slabs+legacy)
  bool       pendingIsNewChunk = true;  // False when rebuild is triggered by block edit
  // BGM output accumulated across incremental MeshGen steps.
  // Lives here so it survives between buildStep() calls (one face-dir per call).
  BinaryGreedyMesher::Output bgmOutput;

  // BGM-based mesh generation (replaces buildNormaly + compress pipeline)
  void buildBGM();

  void flushDrawData(Renderer* t_renderer, StaticPipeline* stapip,
                     std::vector<Vec4>* inVertices,
                     std::vector<Color>* inColors, std::vector<Vec4>* inUVs,
                     int offset, int count, bool needsClipping);

  // Render merged geometry grouped by atlas tile using RegionRepeat.
  void renderGrouped(Renderer* t_renderer, StaticPipeline* stapip,
                     std::vector<Vec4>* pVerts, std::vector<Color>* pColors,
                     std::vector<Vec4>* pUV,
                     const std::vector<TileGroup>& groups, bool needsClipping);

  // Per-tile vertex groups for RegionRepeat rendering
  std::vector<TileGroup> mergedOpaqueGroups;
  std::vector<TileGroup> mergedTranspGroups;

  Vec4 camPositon = Vec4(0, 0, 0);
  int _distanceFromPlayerInChunks = -1;

  // Distance cache optimization
  float cachedDistanceSquared = -1.0f;
  bool distanceCacheDirty = true;

  Plane* frustumPlanes = nullptr;

  bool dirty = false;

  OnLoadedCallback onLoadedCallback;

  // Draw data buffers — persistent storage, populated by buildBGM().
  // Opaque geometry:
  std::vector<Vec4>  vertices;
  std::vector<Vec4>  UV;
  std::vector<Color> colors;
  // Transparent geometry:
  std::vector<Vec4>  transpVertices;
  std::vector<Vec4>  transpUV;
  std::vector<Color> transpColors;
};
