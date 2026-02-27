#include "managers/chunk_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math/plane.hpp"
#include "debug.hpp"
#include <algorithm>
#include <cmath>

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Vec4;

ChunkManager::ChunkManager() : Singleton<ChunkManager>() {
  tickHandles = new TickTaskHandles();
}

ChunkManager::~ChunkManager() {
  delete tickHandles;
  tickHandles = nullptr;
  for (u16 i = 0; i < chunks.size(); i++) {
    delete chunks[i];
    chunks[i] = NULL;
  }
  chunks.clear();
  chunks.shrink_to_fit();

  visibleChunks.clear();
  visibleChunks.shrink_to_fit();

  loadedChunks.clear();
  loadedChunks.shrink_to_fit();
}

void ChunkManager::init(WorldLightModel* t_worldLightModel, Level* level) {
  worldLightModel = t_worldLightModel;
  pLevel = level;
  for (size_t i = 0; i < columnHeightMap.size(); i++) {
    columnHeightMap[i].valid = false;
    columnHeightMap[i].hasBlocks = false;
    columnHeightMap[i].topChunkY = 0;
  }
  this->generateChunks();
  this->populateNeighborCache();  // Phase 4: pre-compute O(1) neighbor lookups for BFS
}

void ChunkManager::clearAllChunks() {
  for (u16 i = 0; i < chunks.size(); i++) chunks[i]->clear();
}

void ChunkManager::updateLoadedChunks() {
  loadedChunks.clear();
  activeChunks.clear();

  for (u16 i = 0; i < chunks.size(); i++) {
    if (chunks[i]->isLoaded() == false) continue;
    loadedChunks.emplace_back(chunks[i]);
    activeChunks.emplace_back(chunks[i]);
  }
}

void ChunkManager::addToLoadedChunks(Chunk* chunk) {
  loadedChunks.push_back(chunk);
  activeChunks.push_back(chunk);
}

void ChunkManager::removeFromLoadedChunks(Chunk* chunk) {
  // Swap-and-pop for O(1) removal (order doesn't matter for these lists)
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    if (loadedChunks[i] == chunk) {
      loadedChunks[i] = loadedChunks.back();
      loadedChunks.pop_back();
      break;
    }
  }
  for (size_t i = 0; i < activeChunks.size(); i++) {
    if (activeChunks[i] == chunk) {
      activeChunks[i] = activeChunks.back();
      activeChunks.pop_back();
      break;
    }
  }
}

void ChunkManager::update(const Plane* frustumPlanes, Vec4* camPos,
                          u8 maxRenderDistance) {
  visibleChunks.clear();
#ifdef DEBUG_MODE
  culledChunks.clear();  // No culling in standard mode
#endif

  Chunk* cameraChunk = getChunkByWorldPosition(*camPos);
  const float maxDistSq =
      static_cast<float>((maxRenderDistance + 1) * (maxRenderDistance + 1)) *
      (CHUNK_SIZE * CHUNK_SIZE);

  // TODO: refactore to fast index by offset
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    Chunk* chk = loadedChunks[i];
    if (chk->isLoaded()) {
      chk->setCamPosition(camPos);
      chk->update(frustumPlanes);

      if (chk->isVisible()) {
        // Distance filter: skip chunks beyond draw distance
        if (cameraChunk) {
          float distSq = horizontalDistance2DSquared(
              cameraChunk->center, chk->center);
          if (distSq > maxDistSq) continue;
        }
        visibleChunks.emplace_back(chk);
      }
    }
  }
}

void ChunkManager::tick() {
  // Note: enqueueChunksToReloadLight() is called from World::tick() at tick 250
  // Removed duplicate call here to prevent queue overflow

  // Phase 2: Only tick active (loaded) chunks instead of all 2048
  // Reduces from 2048 virtual calls to ~100-500 (80-95% reduction)
  for (size_t i = 0; i < activeChunks.size(); i++) {
    activeChunks[i]->tick();
  }
}

void ChunkManager::registerTickCallbacks(TickScheduler& scheduler) {
  // Process every tick (was every 2) - safe with reduced queue size from
  // LOD filtering + removed onLoadedCallback + color-only fast path
  tickHandles->add(scheduler.everyHandle(1, [this]() {
    if (!chunksToUpdateLight.empty()) reloadLightDataAsync();
  }));
}

void ChunkManager::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  for (u16 i = 0; i < visibleChunks.size(); i++)
    visibleChunks[i]->renderer(t_renderer, stapip);
  for (u16 i = 0; i < visibleChunks.size(); i++)
    visibleChunks[i]->rendererTransparentData(t_renderer, stapip);

#ifdef DEBUG_MODE
  if (g_debug_menu.showChunkBorders) {
    for (u16 i = 0; i < visibleChunks.size(); i++) {
      t_renderer->renderer3D.utility.drawBBox(*visibleChunks[i]->bbox,
                                              Color(50, 50, 200));
    }
  }
#endif  // end if DEBUG_MODE
}

void ChunkManager::rendererOpaque(Renderer* t_renderer,
                                  StaticPipeline* stapip) {
  for (u16 i = 0; i < visibleChunks.size(); i++) {
    visibleChunks[i]->renderer(t_renderer, stapip);

#ifdef DEBUG_MODE
    if (g_debug_menu.showChunkBorders) {
      t_renderer->renderer3D.utility.drawBBox(*visibleChunks[i]->bbox,
                                              Color(50, 50, 200));
    }
  }
  
  // Render culled chunks bounding boxes in red
  if (g_debug_menu.showCulledChunks) {
    for (u16 i = 0; i < culledChunks.size(); i++) {
      t_renderer->renderer3D.utility.drawBBox(*culledChunks[i]->bbox,
                                              Color(255, 0, 0));
    }
  }
#endif  // end if DEBUG_MODE
}

void ChunkManager::rendererTransparent(Renderer* t_renderer,
                                       StaticPipeline* stapip) {
  for (u16 i = 0; i < visibleChunks.size(); i++) {
    visibleChunks[i]->rendererTransparentData(t_renderer, stapip);
#ifdef DEBUG_MODE
    if (g_debug_menu.showChunkBorders) {
      t_renderer->renderer3D.utility.drawBBox(*visibleChunks[i]->bbox,
                                              Color(50, 50, 200));
    }
  }
  
  // Render culled chunks bounding boxes in red
  if (g_debug_menu.showCulledChunks) {
    for (u16 i = 0; i < culledChunks.size(); i++) {
      t_renderer->renderer3D.utility.drawBBox(*culledChunks[i]->bbox,
                                              Color(255, 0, 0));
    }
  }
#endif  // end if DEBUG_MODE
}

void ChunkManager::generateChunks() {
  u16 tempId = 0;

  for (size_t x = 0; x < OVERWORLD_MAX_DISTANCE; x += CHUNK_SIZE) {
    for (size_t z = 0; z < OVERWORLD_MAX_DISTANCE; z += CHUNK_SIZE) {
      for (size_t y = 0; y < OVERWORLD_MAX_HEIGH; y += CHUNK_SIZE) {
        Vec4 tempMin = Vec4(x, y, z);
        Vec4 tempMax = Vec4(x + CHUNK_SIZE, y + CHUNK_SIZE, z + CHUNK_SIZE);
        Chunk* tempChunk = new Chunk(tempMin, tempMax, tempId);
        tempChunk->init(pLevel, worldLightModel);
        chunks.emplace_back(tempChunk);

        // Phase 1: Populate spatial grid (16x16 horizontal grid)
        // Calculate grid cell index from XZ coordinates
        const size_t gridX = x / CHUNK_SIZE;
        const size_t gridZ = z / CHUNK_SIZE;
        const size_t gridIndex = gridX * OVERWORLD_H_DISTANCE_IN_CHUNKS + gridZ;
        spatialGrid[gridIndex].push_back(tempChunk);

        tempId++;
      }
    }
  }
};

Chunk* ChunkManager::getChunkById(const u16& id) {
  if (id < chunks.size()) return chunks[id];
  return nullptr;
};

void ChunkManager::enqueueChunksToReloadLight() {
  // Enqueue all visible chunks for light reload.
  // BGM rebuilds are fast enough that distance-based skipping is no longer needed.
  for (size_t i = 0; i < visibleChunks.size(); i++) {
    Chunk* chunk = visibleChunks[i];
    // Use bitset for O(1) duplicate check
    if (!chunksInLightQueue.test(chunk->id)) {
      chunksToUpdateLight.push({chunk, true});  // colors-only for day/night
      chunksInLightQueue.set(chunk->id);
    }
  }
}

void ChunkManager::enqueueChunkToReloadLight(Chunk* chunk, bool colorsOnly) {
  if (chunk && chunk->isLoaded()) {
    // Use bitset for O(1) duplicate check
    if (!chunksInLightQueue.test(chunk->id)) {
      chunksToUpdateLight.push({chunk, colorsOnly});
      chunksInLightQueue.set(chunk->id);
    }
  }
}

void ChunkManager::reloadLightDataAsync() {
  if (chunksToUpdateLight.empty()) return;

  // Process up to LIGHT_UPDATE_BATCH_SIZE chunks per call
  // With ~40 visible chunks enqueued per cycle and processing every 2 ticks,
  // a batch of 4 drains the queue quickly
  constexpr int LIGHT_UPDATE_BATCH_SIZE = 4;
  int processed = 0;

  // Time budget: 2ms to prevent frame stalls from expensive light reloads
  static constexpr u32 LIGHT_BUDGET_CYCLES = 294912u;  // 2ms
  u32 lightStart;
  asm volatile("mfc0 %0, $9" : "=r"(lightStart));

  while (!chunksToUpdateLight.empty() && processed < LIGHT_UPDATE_BATCH_SIZE) {
    auto entry = chunksToUpdateLight.front();
    chunksToUpdateLight.pop();
    chunksInLightQueue.reset(entry.chunk->id);  // Clear bitset entry

    Chunk* chunk = entry.chunk;

    // Validate using direct ID lookup — O(1) instead of O(n) std::find
    if (chunk == nullptr || chunk->id >= chunks.size()) continue;
    if (chunks[chunk->id] != chunk) continue;  // Pointer mismatch = stale

    // Validate state (chunk may have been unloaded)
    if (!chunk->isLoaded()) continue;

    // Skip recently-built chunks - they already have fresh light from build()
    if (g_ticksCounter >= chunk->loadedAtTick &&
        (g_ticksCounter - chunk->loadedAtTick) < 10) continue;

    // Dispatch: colors-only (day/night) vs full rebuild (block change)
    if (entry.colorsOnly) {
      chunk->reloadLightColorsOnly();
    } else {
      chunk->reloadLightData();
    }
    processed++;

    // Check time budget after each reload
    u32 lightNow;
    asm volatile("mfc0 %0, $9" : "=r"(lightNow));
    if ((lightNow - lightStart) >= LIGHT_BUDGET_CYCLES) break;
  }
}

void ChunkManager::reloadLightData() {
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    loadedChunks[i]->reloadLightData();
  }
  clearLightDataQueue();
}

void ChunkManager::enqueueAffectedChunksForLightReload(const Vec4& blockPos,
                                                        float radiusInChunks,
                                                        bool immediateUpdate) {
  // Get chunk containing the changed block
  Chunk* immediateChunk = getChunkByBlockOffset(blockPos);
  
  // Optional: Immediate update for instant visual feedback (1-2ms, imperceptible)
  if (immediateUpdate && immediateChunk && immediateChunk->isLoaded()) {
    immediateChunk->reloadLightData();
  }
  
  // Get all chunks within radius (light propagation range)
  std::vector<Chunk*> affectedChunks;
  getChunksInRadius(blockPos, radiusInChunks, affectedChunks);
  
  // Enqueue affected chunks for async light reload (4 per tick)
  for (Chunk* chunk : affectedChunks) {
    // Skip the immediate chunk if we already updated it
    if (immediateUpdate && chunk == immediateChunk) continue;
    
    enqueueChunkToReloadLight(chunk, false);  // block change = full rebuild
  }
}

// Needed to initiate light in all chunks. The visibleChunks will be available
// after the first update...
void ChunkManager::reloadLightDataOfAllChunks() {
  for (size_t i = 0; i < chunks.size(); i++) {
    if (chunks[i]->isLoaded()) chunks[i]->reloadLightData();
  }
  clearLightDataQueue();
}

// void ChunkManager::sortDrawDataFromCamPos(const Vec4& cameraPos) {
//   for (size_t i = 0; i < visibleChunks.size(); i++) {
//     visibleChunks[i]->sortTransParentDrawData(cameraPos);
//   }
// }

const uint16_t ChunkManager::getChunkIdByPosition(
    const Vec4& chunkMinPosition) {
  const Vec4 offset = chunkMinPosition / CHUNK_SIZE;
  return getChunkIdByOffset(offset);
}

const uint16_t ChunkManager::getChunkIdByOffset(const Vec4& chunkMinOffset) {
  const Vec4 pos = chunkMinOffset;
  const uint16_t row = pos.y;
  const uint16_t column = pos.z * OVERWORLD_V_DISTANCE_IN_CHUNKS;
  const uint16_t page = pos.x * OVERWORLD_PAGE_IN_CHUNKS;

  return page + column + row;
}

Vec4 ChunkManager::getChunkPosById(const uint16_t& id) {
  const int x = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS);
  const int z = static_cast<int>((id - (x * OVERWORLD_PAGE_IN_CHUNKS)) /
                                 OVERWORLD_V_DISTANCE_IN_CHUNKS);
  const int y = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS);

  return Vec4(x, y, z) * CHUNK_SIZE;
}

void ChunkManager::getChunkPosById(const uint16_t& id, Vec4* result) {
  const int offsetX = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS);
  const int offsetZ = static_cast<int>((id - (offsetX * OVERWORLD_PAGE_IN_CHUNKS)) /
                                       OVERWORLD_V_DISTANCE_IN_CHUNKS);
  const int offsetY = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS);

  result->x = offsetX * CHUNK_SIZE;
  result->y = offsetY * CHUNK_SIZE;
  result->z = offsetZ * CHUNK_SIZE;
}

Chunk* ChunkManager::getChunkByPosition(const Vec4& chunkMinPosition) {
  const uint16_t id = getChunkIdByPosition(chunkMinPosition);

  if (id < chunks.size()) {
    return chunks[id];
  } else {
    return nullptr;
  }
}

Chunk* ChunkManager::getChunkByOffset(const Vec4& chunkMinOffset) {
  const uint16_t id = getChunkIdByOffset(chunkMinOffset);

  if (id < chunks.size()) {
    return chunks[id];
  } else {
    return nullptr;
  }
}

Chunk* ChunkManager::getChunkByWorldPosition(const Vec4& pos) {
  Vec4 offset = (pos / DOUBLE_BLOCK_SIZE) / CHUNK_SIZE;
  Vec4 tempChunkMin =
      Vec4(std::floor(offset.x), std::floor(offset.y), std::floor(offset.z)) *
      CHUNK_SIZE;

  return getChunkByPosition(tempChunkMin);
}

Chunk* ChunkManager::getChunkByBlockOffset(const Vec4& offset) {
  Vec4 _offset = offset / CHUNK_SIZE;
  Vec4 tempChunkMin = Vec4(std::floor(_offset.x), std::floor(_offset.y),
                           std::floor(_offset.z)) *
                      CHUNK_SIZE;

  return getChunkByPosition(tempChunkMin);
}

int ChunkManager::getHeightAtOffset(const Vec4& offset) {
  int y = pLevel->map.height - 1;

  while (y >= 0) {
    if (pLevel->GetBlockFromMap(offset.x, y - 1, offset.z) !=
        (int)Blocks::AIR_BLOCK)
      return y;
    y--;
  }

  return 0;
}

float ChunkManager::getHeightAtPosition(const Vec4& position) {
  const Vec4 offset = pLevel->worldPosToOffset(position);
  return getHeightAtOffset(offset) * DOUBLE_BLOCK_SIZE;
}

void ChunkManager::getColumnHeightInfo(int chunkX, int chunkZ,
                                       u8& outTopChunkY,
                                       bool& outHasBlocks) {
  const int clampedX = std::max(0, std::min(chunkX,
                                            static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1)));
  const int clampedZ = std::max(0, std::min(chunkZ,
                                            static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1)));
  const size_t gridIndex =
      clampedX * OVERWORLD_H_DISTANCE_IN_CHUNKS + clampedZ;

  ColumnHeightInfo& info = columnHeightMap[gridIndex];
  if (!info.valid) {
    const int startX = clampedX * CHUNK_SIZE;
    const int startZ = clampedZ * CHUNK_SIZE;
    bool found = false;
    int highestBlockY = 0;

    for (int y = OVERWORLD_V_DISTANCE - 1; y >= 0; y--) {
      for (int z = startZ; z < startZ + CHUNK_SIZE; z++) {
        for (int x = startX; x < startX + CHUNK_SIZE; x++) {
          const u8 blockId = pLevel->GetBlockFromMap(x, y, z);
          if (blockId > static_cast<u8>(Blocks::AIR_BLOCK)) {
            found = true;
            highestBlockY = y;
            break;
          }
        }
        if (found) break;
      }
      if (found) break;
    }

    info.valid = true;
    info.hasBlocks = found;
    info.topChunkY = found ? static_cast<u8>(highestBlockY / CHUNK_SIZE) : 0;
  }

  outTopChunkY = info.topChunkY;
  outHasBlocks = info.hasBlocks;
}

void ChunkManager::getChunksInRadius(const Vec4& center, float radiusInChunks,
                                     std::vector<Chunk*>& outChunks) {
  outChunks.clear();

  // Convert center position to chunk grid coordinates
  const float centerChunkX = center.x / CHUNK_SIZE;
  const float centerChunkZ = center.z / CHUNK_SIZE;

  // Calculate grid bounds to check (with safety clamping)
  const int minGridX = std::max(0, static_cast<int>(centerChunkX - radiusInChunks - 1));
  const int maxGridX = std::min(static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1),
                                 static_cast<int>(centerChunkX + radiusInChunks + 1));
  const int minGridZ = std::max(0, static_cast<int>(centerChunkZ - radiusInChunks - 1));
  const int maxGridZ = std::min(static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1),
                                 static_cast<int>(centerChunkZ + radiusInChunks + 1));

  // Squared radius for distance comparisons (avoids sqrt)
  const float radiusSquared = radiusInChunks * radiusInChunks;

  // Iterate only the grid cells within the bounding box
  for (int gridX = minGridX; gridX <= maxGridX; gridX++) {
    for (int gridZ = minGridZ; gridZ <= maxGridZ; gridZ++) {
      const size_t gridIndex = gridX * OVERWORLD_H_DISTANCE_IN_CHUNKS + gridZ;

      // Process all vertical chunks in this XZ column
      for (Chunk* chunk : spatialGrid[gridIndex]) {
        // Validate chunk pointer
        if (chunk == nullptr) continue;

        // 2D distance check (XZ plane only, ignoring Y)
        const float distSquared = horizontalDistance2DSquared(center, chunk->center);
        const float distInChunksSquared = distSquared / (CHUNK_SIZE * CHUNK_SIZE);

        if (distInChunksSquared <= radiusSquared) {
          outChunks.push_back(chunk);
        }
      }
    }
  }
}

Chunk* ChunkManager::getNeighborChunk(Chunk* chunk, u8 face) {
  int dx, dy, dz;
  GetFaceDirection(face, dx, dy, dz);

  // Calculate neighbor's minOffset in block coordinates
  float nx = chunk->minOffset.x + dx;
  float ny = chunk->minOffset.y + dy;
  float nz = chunk->minOffset.z + dz;

  // Bounds check — world is [0, OVERWORLD_H_DISTANCE) × [0, OVERWORLD_V_DISTANCE) × [0, OVERWORLD_H_DISTANCE)
  if (nx < 0 || nx >= OVERWORLD_H_DISTANCE || ny < 0 ||
      ny >= OVERWORLD_V_DISTANCE || nz < 0 || nz >= OVERWORLD_H_DISTANCE) {
    return nullptr;
  }

  Vec4 neighborOffset(nx, ny, nz);
  return getChunkByPosition(neighborOffset);
}

void ChunkManager::populateNeighborCache() {
  // Phase 4: For every chunk, fill neighbors[6] with direct pointers.
  // Called once after generateChunks(). O(N*6) time, O(1) per BFS lookup thereafter.
  // Face indices: 0=TOP(+Y), 1=BOTTOM(-Y), 2=LEFT(+X), 3=RIGHT(-X), 4=FRONT(-Z), 5=BACK(+Z)
  for (size_t i = 0; i < chunks.size(); i++) {
    Chunk* chunk = chunks[i];
    for (u8 face = 0; face < FACE_COUNT; face++) {
      chunk->neighbors[face] = getNeighborChunk(chunk, face);
    }
  }
}

// BFS queue element for visibility graph traversal
struct VisBfsEntry {
  u16 chunkId;
  u8 entryFace;  // Face through which we entered this chunk
  u8 steps;      // Number of steps from camera chunk
};

void ChunkManager::updateWithVisibilityGraph(const Plane* frustumPlanes,
                                              Vec4* camPos,
                                              const Vec4& camForward,
                                              u8 maxRenderDistance) {
  // camForward is used for N·V directional filtering in BFS expansion
  visibleChunks.clear();
  occludedChunksToUnload.clear();

  // Pre-compute distance limit for render culling (with +1 margin to avoid pop-in)
  const float maxRenderDistSq =
      static_cast<float>((maxRenderDistance + 1) * (maxRenderDistance + 1)) *
      (CHUNK_SIZE * CHUNK_SIZE);

#ifdef DEBUG_MODE
  // Start with all loaded chunks as potentially culled
  culledChunks.clear();
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    Chunk* chk = loadedChunks[i];
    if (chk->isLoaded()) {
      culledChunks.push_back(chk);
    }
  }
#endif

  // First, update frustum check for all loaded chunks (needed for filtering)
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    Chunk* chk = loadedChunks[i];
    if (chk->isLoaded()) {
      chk->setCamPosition(camPos);
      chk->update(frustumPlanes);
    }
  }

  // Find the camera chunk
  Chunk* cameraChunk = getChunkByWorldPosition(*camPos);
  if (!cameraChunk || !cameraChunk->isLoaded()) {
    // Fallback to standard frustum culling if camera chunk is not loaded
    for (size_t i = 0; i < loadedChunks.size(); i++) {
      Chunk* chk = loadedChunks[i];
      if (chk->isLoaded() && chk->isVisible()) {
        // Distance filter in fallback path too
        float distSq = horizontalDistance2DSquared(
            chk->center, Vec4(camPos->x / DOUBLE_BLOCK_SIZE,
                              0, camPos->z / DOUBLE_BLOCK_SIZE));
        if (distSq <= maxRenderDistSq)
          visibleChunks.emplace_back(chk);
      }
    }
    return;
  }

  // BFS traversal using visibility graph
  // Link MAX_STEPS to actual draw distance to prevent over-traversal
  const u8 MAX_STEPS = static_cast<u8>(
      std::min(static_cast<int>(maxRenderDistance) + 2, 16));
  static constexpr u16 BFS_QUEUE_SIZE = OVERWORLD_SIZE_IN_CHUNKS;

  // Visited bitset — one bit per chunk ID
  static std::bitset<OVERWORLD_SIZE_IN_CHUNKS> visited;
  visited.reset();

  // Fixed-size circular buffer BFS queue
  static VisBfsEntry bfsQueue[BFS_QUEUE_SIZE];
  u16 qHead = 0;
  u16 qTail = 0;
  u16 qCount = 0;

  // Enqueue camera chunk (special: enters from ALL faces)
  visited.set(cameraChunk->id);
  visibleChunks.emplace_back(cameraChunk);

  // Queue neighbors from camera chunk directly (no connectivity filter for
  // camera chunk)
  for (u8 face = 0; face < FACE_COUNT; face++) {
    Chunk* neighbor = cameraChunk->neighbors[face];  // Phase 4: O(1) pointer lookup
    if (!neighbor || !neighbor->isLoaded()) continue;
    if (visited.test(neighbor->id)) continue;

    // Frustum check
    if (!neighbor->isVisible()) continue;

    // Distance filter
    float distSq = horizontalDistance2DSquared(
        cameraChunk->center, neighbor->center);
    if (distSq > maxRenderDistSq) continue;

    visited.set(neighbor->id);
    bfsQueue[qTail] = {neighbor->id, OppositeFace(face), 1};
    qTail = (qTail + 1) % BFS_QUEUE_SIZE;
    qCount++;
  }

  // BFS traversal with time budget to prevent frame stalls
  static constexpr u32 BFS_BUDGET_CYCLES = 294912u;  // 2ms
  u32 bfsStart;
  asm volatile("mfc0 %0, $9" : "=r"(bfsStart));
  u8 bfsIterCount = 0;

  while (qCount > 0) {
    VisBfsEntry entry = bfsQueue[qHead];
    qHead = (qHead + 1) % BFS_QUEUE_SIZE;
    qCount--;

    Chunk* current = chunks[entry.chunkId];
    if (!current->isLoaded()) continue;

    // Distance filter: skip chunks beyond draw distance
    float distSq = horizontalDistance2DSquared(
        cameraChunk->center, current->center);
    if (distSq > maxRenderDistSq) continue;

    // Add to visible chunks
    visibleChunks.emplace_back(current);

    // Don't expand further if we've reached the step limit
    if (entry.steps >= MAX_STEPS) continue;

    // Try to expand to all 6 neighbors
    for (u8 exitFace = 0; exitFace < FACE_COUNT; exitFace++) {
      // Filter 1: No backtracking — don't go back the way we came
      if (exitFace == entry.entryFace) continue;

      // Filter 2: N·V directional check (horizontal only).
      // The article's N·V < 0 filter prevents BFS from expanding backward.
      // We only apply this to horizontal faces (NORTH/SOUTH/EAST/WEST).
      // Vertical faces (TOP/BOTTOM) are exempt because with only 4 vertical
      // chunk layers, blocking vertical expansion causes missing terrain.
      if (exitFace < FACE_TOP) {  // NORTH=0, SOUTH=1, EAST=2, WEST=3
        float fnx, fny, fnz;
        GetFaceNormalVec(exitFace, fnx, fny, fnz);
        float dot = fnx * camForward.x + fnz * camForward.z;
        if (dot < 0.0f) continue;  // Exit direction opposes camera = going backward
      }

      // Filter 3: Connectivity test — can we see through this chunk from
      // entryFace to exitFace?
      if (!current->isConnected(entry.entryFace, exitFace)) continue;

      Chunk* neighbor = current->neighbors[exitFace];  // Phase 4: O(1) pointer lookup
      if (!neighbor || !neighbor->isLoaded()) continue;
      if (visited.test(neighbor->id)) continue;

      // Filter 4: Frustum check
      if (!neighbor->isVisible()) continue;

      // Filter 5: Distance check — skip chunks beyond draw distance
      float neighborDistSq = horizontalDistance2DSquared(
          cameraChunk->center, neighbor->center);
      if (neighborDistSq > maxRenderDistSq) continue;

      // Step cost with heuristic penalties (from Tomcc's "More filters!" section)
      u8 stepCost = 1;

      // Heuristic: Going down below sea level costs +1 step
      // Underground chunks are likely cave paths that should be pruned earlier
      if (exitFace == FACE_BOTTOM && neighbor->minOffset.y < SEA_LEVEL_Y) {
        stepCost += 1;
      }

      u8 newSteps = entry.steps + stepCost;

      // Filter 6: Step budget
      if (newSteps > MAX_STEPS) continue;

      visited.set(neighbor->id);
      bfsQueue[qTail] = {neighbor->id, OppositeFace(exitFace), newSteps};
      qTail = (qTail + 1) % BFS_QUEUE_SIZE;
      qCount++;
    }

    // Check time budget every 8 iterations to avoid BFS stalls
    if (++bfsIterCount >= 8) {
      bfsIterCount = 0;
      u32 bfsNow;
      asm volatile("mfc0 %0, $9" : "=r"(bfsNow));
      if ((bfsNow - bfsStart) >= BFS_BUDGET_CYCLES) break;
    }
  }

  // Increased from 60 to 90 frames (~3 sec at 30 FPS) to be more conservative
  // and reduce fighting with chunk scheduling
  static constexpr u8 OCCLUDED_FRAMES_TO_UNLOAD = 90;
  static constexpr int HALO_DISTANCE = 1;

  for (Chunk* chunk : loadedChunks) {
    if (!chunk->isLoaded()) continue;

    if (chunk->id == cameraChunk->id) {
      chunk->consecutiveOccludedFrames = 0;
      continue;
    }

    const int dx = static_cast<int>((chunk->minOffset.x - cameraChunk->minOffset.x) / CHUNK_SIZE);
    const int dz = static_cast<int>((chunk->minOffset.z - cameraChunk->minOffset.z) / CHUNK_SIZE);
    if (std::abs(dx) <= HALO_DISTANCE && std::abs(dz) <= HALO_DISTANCE) {
      chunk->consecutiveOccludedFrames = 0;
      continue;
    }

    if (visited.test(chunk->id) || chunk->isVisible()) {
      chunk->consecutiveOccludedFrames = 0;
      continue;
    }

    if (chunk->consecutiveOccludedFrames < 255) {
      chunk->consecutiveOccludedFrames++;
    }

    if (chunk->consecutiveOccludedFrames >= OCCLUDED_FRAMES_TO_UNLOAD) {
      occludedChunksToUnload.push_back(chunk);
      chunk->consecutiveOccludedFrames = 0;
    }
  }

#ifdef DEBUG_MODE
  // Rebuild culledChunks to contain only chunks that were NOT visited
  std::vector<Chunk*> actualCulledChunks;
  actualCulledChunks.reserve(culledChunks.size());
  for (Chunk* chunk : culledChunks) {
    if (!visited.test(chunk->id)) {
      actualCulledChunks.push_back(chunk);
    }
  }
  culledChunks = std::move(actualCulledChunks);
#endif
}
