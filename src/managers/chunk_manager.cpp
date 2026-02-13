#include "managers/chunk_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math/plane.hpp"
#include "debug.hpp"
#include <algorithm>
#include <cmath>

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Vec4;

ChunkManager::ChunkManager() : Singleton<ChunkManager>() {}

ChunkManager::~ChunkManager() {
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
}

void ChunkManager::clearAllChunks() {
  for (u16 i = 0; i < chunks.size(); i++) chunks[i]->clear();
}

void ChunkManager::updateLoadedChunks() {
  loadedChunks.clear();
  activeChunks.clear();  // Phase 2: Maintain activeChunks list
  
  // Phase 1: spatialGrid is STATIC - never remove chunks from it
  // It's populated once in generateChunks() and stays constant
  // Validation happens at usage time in getChunksInRadius()
  
  for (u16 i = 0; i < chunks.size(); i++) {
    if (chunks[i]->isLoaded() == false) continue;
    loadedChunks.emplace_back(chunks[i]);
    activeChunks.emplace_back(chunks[i]);  // Phase 2: Same as loadedChunks for now
  }
}

void ChunkManager::update(const Plane* frustumPlanes, Vec4* camPos) {
  visibleChunks.clear();
#ifdef DEBUG_MODE
  culledChunks.clear();  // No culling in standard mode
#endif

  // TODO: refactore to fast index by offset
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    Chunk* chk = loadedChunks[i];
    if (chk->isLoaded()) {
      chk->setCamPosition(camPos);
      chk->update(frustumPlanes);

      if (chk->isVisible()) visibleChunks.emplace_back(chk);
    }
  }
}

void ChunkManager::tick() {
  if (isTicksCounterAt(2)) {
    if (chunksToUpdateLight.empty() == false) reloadLightDataAsync();
  }

  // Note: enqueueChunksToReloadLight() is called from World::tick() at tick 250
  // Removed duplicate call here to prevent queue overflow

  // Phase 2: Only tick active (loaded) chunks instead of all 2048
  // Reduces from 2048 virtual calls to ~100-500 (80-95% reduction)
  for (size_t i = 0; i < activeChunks.size(); i++) {
    activeChunks[i]->tick();
  }
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
  // Only enqueue visible chunks — invisible chunks will get updated
  // via onLoadedCallback when they become visible/loaded again.
  // This reduces queue input from ~305 (all loaded) to ~40 (visible).
  for (size_t i = 0; i < visibleChunks.size(); i++) {
    Chunk* chunk = visibleChunks[i];
    // Use bitset for O(1) duplicate check
    if (!chunksInLightQueue.test(chunk->id)) {
      chunksToUpdateLight.push(chunk);
      chunksInLightQueue.set(chunk->id);
    }
  }
}

void ChunkManager::enqueueChunkToReloadLight(Chunk* chunk) {
  if (chunk && chunk->isLoaded()) {
    // Use bitset for O(1) duplicate check
    if (!chunksInLightQueue.test(chunk->id)) {
      chunksToUpdateLight.push(chunk);
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

  while (!chunksToUpdateLight.empty() && processed < LIGHT_UPDATE_BATCH_SIZE) {
    auto chunk = chunksToUpdateLight.front();
    chunksToUpdateLight.pop();
    chunksInLightQueue.reset(chunk->id);  // Clear bitset entry

    // Validate using direct ID lookup — O(1) instead of O(n) std::find
    if (chunk == nullptr || chunk->id >= chunks.size()) continue;
    if (chunks[chunk->id] != chunk) continue;  // Pointer mismatch = stale

    // Validate state (chunk may have been unloaded)
    if (!chunk->isLoaded()) continue;

    chunk->reloadLightData();
    processed++;
  }
}

void ChunkManager::reloadLightData() {
  for (size_t i = 0; i < loadedChunks.size(); i++) {
    loadedChunks[i]->reloadLightData();
  }
  clearLightDataQueue();
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
  result->x = static_cast<int>(id / OVERWORLD_PAGE_IN_CHUNKS) * CHUNK_SIZE;
  result->z = static_cast<int>((id - (result->x * OVERWORLD_PAGE_IN_CHUNKS)) /
                               OVERWORLD_V_DISTANCE_IN_CHUNKS) *
              CHUNK_SIZE;
  result->y = (id % OVERWORLD_V_DISTANCE_IN_CHUNKS) * CHUNK_SIZE;
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
        
        // Skip only Unloading chunks - Building chunks need to be returned
        // so scheduleChunksNeighbors can process them
        if (chunk->getState() == ChunkState::Unloading) continue;
        
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

// BFS queue element for visibility graph traversal
struct VisBfsEntry {
  u16 chunkId;
  u8 entryFace;  // Face through which we entered this chunk
  u8 steps;      // Number of steps from camera chunk
};

void ChunkManager::updateWithVisibilityGraph(const Plane* frustumPlanes,
                                              Vec4* camPos,
                                              const Vec4& camForward) {
  static_cast<void>(camForward);
  visibleChunks.clear();
  occludedChunksToUnload.clear();

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
        visibleChunks.emplace_back(chk);
      }
    }
    return;
  }

  // BFS traversal using visibility graph
  // Increased MAX_STEPS from 16 to 24 to prevent culling forward chunks with turns
  // With MAX_DRAW_DISTANCE=16 and turn cost=2, 16 steps is too restrictive
  static constexpr u8 MAX_STEPS = 24;
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
    Chunk* neighbor = getNeighborChunk(cameraChunk, face);
    if (!neighbor || !neighbor->isLoaded()) continue;
    if (visited.test(neighbor->id)) continue;

    // Frustum check
    if (!neighbor->isVisible()) continue;

    visited.set(neighbor->id);
    bfsQueue[qTail] = {neighbor->id, OppositeFace(face), 1};
    qTail = (qTail + 1) % BFS_QUEUE_SIZE;
    qCount++;
  }

  // BFS traversal
  while (qCount > 0) {
    VisBfsEntry entry = bfsQueue[qHead];
    qHead = (qHead + 1) % BFS_QUEUE_SIZE;
    qCount--;

    Chunk* current = chunks[entry.chunkId];
    if (!current->isLoaded()) continue;

    // Add to visible chunks
    visibleChunks.emplace_back(current);

    // Don't expand further if we've reached the step limit
    if (entry.steps >= MAX_STEPS) continue;

    // Try to expand to all 6 neighbors
    for (u8 exitFace = 0; exitFace < FACE_COUNT; exitFace++) {
      // Filter 1: Connectivity test — can we see through this chunk from
      // entryFace to exitFace?
      if (!current->isConnected(entry.entryFace, exitFace)) continue;

      // Filter 2: No backtracking — don't go back the way we came
      if (exitFace == OppositeFace(entry.entryFace)) continue;

      Chunk* neighbor = getNeighborChunk(current, exitFace);
      if (!neighbor || !neighbor->isLoaded()) continue;
      if (visited.test(neighbor->id)) continue;

      // Filter 3: Frustum check
      if (!neighbor->isVisible()) continue;

      visited.set(neighbor->id);

      // Step cost: 1 for straight movement, 2 for turns
      u8 stepCost = (exitFace == entry.entryFace) ? 1 : 2;
      u8 newSteps = entry.steps + stepCost;

      // Filter 4: Step budget
      if (newSteps > MAX_STEPS) continue;

      bfsQueue[qTail] = {neighbor->id, OppositeFace(exitFace), newSteps};
      qTail = (qTail + 1) % BFS_QUEUE_SIZE;
      qCount++;
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
