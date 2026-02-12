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
#include <array>
#include <queue>
#include <bitset>
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
  void updateWithVisibilityGraph(const Plane* frustumPlanes, Vec4* camPos,
                                  const Vec4& camForward);
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
  std::vector<Chunk*>* getVisibleChunks() { return &visibleChunks; };
#ifdef DEBUG_MODE
  std::vector<Chunk*>* getCulledChunks() { return &culledChunks; };
#endif

  void enqueueChunkToReloadLight(Chunk* chunk);  // Single chunk enqueue
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

  // Get the chunk neighboring the given chunk on the specified face
  Chunk* getNeighborChunk(Chunk* chunk, u8 face);

  // Helper for 2D horizontal distance calculation (XZ plane only)
  static inline float horizontalDistance2D(const Vec4& a, const Vec4& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;

    // TODO: reimplement using fast sqrt approximation if needed or VU code
    return sqrtf(dx * dx + dz * dz);
  }

  // Optimized: Squared distance (avoids sqrt, preserves ordering for sorting)
  static inline float horizontalDistance2DSquared(const Vec4& a, const Vec4& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return dx * dx + dz * dz;
  }

  // Spatial optimization: Get chunks within radius using 2D grid
  void getChunksInRadius(const Vec4& center, float radiusInChunks,
                         std::vector<Chunk*>& outChunks);

 private:
  WorldLightModel* worldLightModel;
  Level* pLevel;

  std::queue<Chunk*> chunksToUpdateLight;
  std::vector<Chunk*> chunks;
  std::vector<Chunk*> loadedChunks;
  std::vector<Chunk*> visibleChunks;
  std::vector<Chunk*> activeChunks;  // Phase 2: Chunks with state == Loaded for faster tick
#ifdef DEBUG_MODE
  std::vector<Chunk*> culledChunks;  // Chunks removed by cave culling
#endif

  // Phase 1: Spatial grid for optimized radius queries (16x16 horizontal grid)
  // Each cell contains pointers to the 8 vertical chunks in that XZ column
  static constexpr size_t SPATIAL_GRID_SIZE = OVERWORLD_H_DISTANCE_IN_CHUNKS * OVERWORLD_H_DISTANCE_IN_CHUNKS;
  std::array<std::vector<Chunk*>, SPATIAL_GRID_SIZE> spatialGrid;

  void generateChunks();

  void reloadLightDataAsync();
  void clearLightDataQueue() {
    while (!chunksToUpdateLight.empty()) chunksToUpdateLight.pop();
  };

  const uint16_t getChunkIdByPosition(const Vec4& chunkMinPosition);
  const uint16_t getChunkIdByOffset(const Vec4& chunkMinOffset);
  u8 isTimeToUpdateLight = 0;
};
