#include "managers/chunk_manager.hpp"
#include "managers/tick_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/dma_gif_builder.hpp"
#include "managers/background_task_service.hpp"
#include "math/plane.hpp"
#include "debug.hpp"
#include <algorithm>
#include <cmath>
#include <dma.h>

using Tyra::M4x4;
using Tyra::Plane;
using Tyra::Vec4;

ChunkManager::ChunkManager() : Singleton<ChunkManager>() {
  tickHandles = new TickTaskHandles();
  for (size_t i = 0; i < OVERWORLD_SIZE_IN_CHUNKS; i++) {
    chunks[i] = nullptr;
  }
}

ChunkManager::~ChunkManager() {
  delete tickHandles;
  tickHandles = nullptr;
  for (u16 i = 0; i < OVERWORLD_SIZE_IN_CHUNKS; i++) {
    if (chunks[i] != nullptr) {
      delete chunks[i];
      chunks[i] = nullptr;
    }
  }
  // chunks is an array, no need to clear or shrink

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
  this->populateNeighborCache();  // Phase 4: pre-compute O(1) neighbor lookups
                                  // for BFS
}

void ChunkManager::clearAllChunks() {
  for (u16 i = 0; i < OVERWORLD_SIZE_IN_CHUNKS; i++) {
    if (chunks[i]) chunks[i]->clear();
  }
}

void ChunkManager::updateLoadedChunks() {
  loadedChunks.clear();
  activeChunks.clear();

  for (u16 i = 0; i < OVERWORLD_SIZE_IN_CHUNKS; i++) {
    if (chunks[i] == nullptr || chunks[i]->isLoaded() == false) continue;
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
                          u8 maxRenderDistance, const float& deltaTime) {
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
      chk->update(frustumPlanes, deltaTime);

      if (chk->isVisible()) {
        // Distance filter: skip chunks beyond draw distance
        if (cameraChunk) {
          float distSq =
              horizontalDistance2DSquared(cameraChunk->center, chk->center);
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

Chunk* ChunkManager::requestSpawnChunk(const uint16_t& id) {
  return spawnChunk(id);
}

void ChunkManager::requestDespawnChunk(const uint16_t& id) {
  despawnChunk(id);
}

void ChunkManager::renderer(Renderer* t_renderer, StaticPipeline* stapip) {
  // Ensure blocks texture is bound before rendering any chunks, since chunk
  BlockManager* blockMgr = BlockManager::getInstance();
  Tyra::Texture* tex = blockMgr->getBlocksTexture();
  t_renderer->core.texture.useTexture(tex);

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
#else
  }
#endif  // end if DEBUG_MODE

  // Phase 2 optimisation: renderGrouped() defers the default-wrap restore
  // to avoid one dma_channel_wait per chunk. We do a single restore here
  // after all opaque chunks have been drawn.
  if (!visibleChunks.empty()) {
    dma_channel_wait(DMA_CHANNEL_VIF1, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 0);
    Tyra::Texture* tex = BlockManager::getInstance()->getBlocksTexture();
    tex->setDefaultWrapSettings();
    static DmaGifBuilder clampRestoreBuilder;
    // Build a minimal GIF packet for the CLAMP register restore
    const texwrap_t* wrap = tex->getWrapSettings();
    clampRestoreBuilder.begin();
    clampRestoreBuilder.addGifTag(GIF_REG_AD);
    clampRestoreBuilder.addAd(
        GS_SET_CLAMP(wrap->horizontal, wrap->vertical, wrap->minu, wrap->maxu,
                     wrap->minv, wrap->maxv),
        GS_REG_CLAMP_1);
    clampRestoreBuilder.send();
  }
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
#else
  }
#endif  // end if DEBUG_MODE

  // Phase 2 optimisation: same deferred CLAMP restore for transparent pass.
  if (!visibleChunks.empty()) {
    dma_channel_wait(DMA_CHANNEL_VIF1, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 0);
    Tyra::Texture* tex = BlockManager::getInstance()->getBlocksTexture();
    tex->setDefaultWrapSettings();
    static DmaGifBuilder clampRestoreBuilder;
    const texwrap_t* wrap = tex->getWrapSettings();
    clampRestoreBuilder.begin();
    clampRestoreBuilder.addGifTag(GIF_REG_AD);
    clampRestoreBuilder.addAd(
        GS_SET_CLAMP(wrap->horizontal, wrap->vertical, wrap->minu, wrap->maxu,
                     wrap->minv, wrap->maxv),
        GS_REG_CLAMP_1);
    clampRestoreBuilder.send();
  }
}

void ChunkManager::generateChunks() {
  // Chunks are now spawned dynamically in Phase 3.
  // We keep the spatialGrid populated with nullptrs or just initialize it.
  for (size_t i = 0; i < OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD; i++) {
    spatialGrid[i].clear();
  }
}

Chunk* ChunkManager::spawnChunk(const uint16_t& id) {
  if (id >= OVERWORLD_SIZE_IN_CHUNKS) return nullptr;
  if (chunks[id] != nullptr) return chunks[id];

  Vec4 pos = getChunkPosById(id);
  Vec4 tempMax = pos + Vec4(CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE);
  
  Chunk* tempChunk = new Chunk(pos, tempMax, id);
  tempChunk->init(pLevel, worldLightModel);
  chunks[id] = tempChunk;

  // Populate spatial grid
  const size_t gridX = static_cast<size_t>(pos.x) / CHUNK_SIZE;
  const size_t gridZ = static_cast<size_t>(pos.z) / CHUNK_SIZE;
  const size_t gridIndex = gridX * OVERWORLD_H_DISTANCE_IN_CHUNKS + gridZ;
  
  bool found = false;
  for (auto* c : spatialGrid[gridIndex]) {
    if (c == tempChunk) { found = true; break; }
  }
  if (!found) spatialGrid[gridIndex].push_back(tempChunk);

  // Link neighbors
  for (u8 face = 0; face < FACE_COUNT; face++) {
    Chunk* neighbor = getNeighborChunk(tempChunk, face);
    if (neighbor) {
      tempChunk->neighbors[face] = neighbor;
      // Link back: neighbor's opposite face points to us
      u8 oppositeFace = OppositeFace(face);
      neighbor->neighbors[oppositeFace] = tempChunk;
      // Mark neighbor as dirty so it rebuilds meshes with new boundary faces
      neighbor->markDirty();
    }
  }

  return tempChunk;
}

void ChunkManager::despawnChunk(const uint16_t& id) {
  if (id >= OVERWORLD_SIZE_IN_CHUNKS || chunks[id] == nullptr) return;
  
  Chunk* chunk = chunks[id];
  
  // Unlink neighbors
  for (u8 face = 0; face < FACE_COUNT; face++) {
    Chunk* neighbor = chunk->neighbors[face];
    if (neighbor) {
      u8 oppositeFace = OppositeFace(face);
      neighbor->neighbors[oppositeFace] = nullptr;
      neighbor->markDirty();
    }
  }

  // Remove from lists
  removeFromLoadedChunks(chunk);
  
  const size_t gridX = static_cast<size_t>(chunk->minOffset.x) / CHUNK_SIZE;
  const size_t gridZ = static_cast<size_t>(chunk->minOffset.z) / CHUNK_SIZE;
  const size_t gridIndex = gridX * OVERWORLD_H_DISTANCE_IN_CHUNKS + gridZ;
  
  auto& gridList = spatialGrid[gridIndex];
  gridList.erase(std::remove(gridList.begin(), gridList.end(), chunk), gridList.end());
  
  delete chunks[id];
  chunks[id] = nullptr;
}

Chunk* ChunkManager::getChunkById(const u16& id) {
  if (id < OVERWORLD_SIZE_IN_CHUNKS) return chunks[id];
  return nullptr;
};

void ChunkManager::enqueueChunksToReloadLight() {
  // Enqueue all visible chunks for light reload.
  // BGM rebuilds are fast enough that distance-based skipping is no longer
  // needed.
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
      if (!colorsOnly) {
        chunksNeedingFullRebuild.set(chunk->id);
      }
    } else {
      // Chunk already in queue — check if we need to upgrade flag
      // Allow colorsOnly=true → colorsOnly=false upgrade
      if (!colorsOnly) {
        chunksNeedingFullRebuild.set(chunk->id);
      }
    }
  }
}

void ChunkManager::reloadLightDataAsync() {
  if (chunksToUpdateLight.empty()) return;

  // Process up to LIGHT_UPDATE_BATCH_SIZE chunks per call.
  // A batch of 4 with 2ms budget keeps block-edit feedback snappy (1-2 ticks
  // for nearby chunks) while not stalling the frame.
  constexpr int LIGHT_UPDATE_BATCH_SIZE = 4;
  // Color-only relights are cheap (~0.2ms each), so we can process more per tick.
  // With Step 5 optimizations, most chunks are now colorsOnly → faster queue drain.
  constexpr int MAX_COLORS_ONLY_PER_TICK = 4;
  // Keep block edit feedback responsive while still bounded.
  constexpr int MAX_FULL_REBUILDS_PER_TICK = 1;
  int processed = 0;
  int processedColorsOnly = 0;
  int processedFullRebuild = 0;

  // Time budget: 2ms expressed in EE COP0 Count ticks (~147.456 MHz)
  static constexpr u32 LIGHT_BUDGET_CYCLES = 294912u;  // 2ms
  // Dedicated colors-only budget (1ms) to spread day/night updates smoothly.
  static constexpr u32 COLORS_ONLY_BUDGET_CYCLES = 147456u;  // 1ms
  u32 lightStart;
  asm volatile("mfc0 %0, $9" : "=r"(lightStart));
  const u32 colorsOnlyStart = lightStart;

  while (!chunksToUpdateLight.empty() && processed < LIGHT_UPDATE_BATCH_SIZE) {
    auto entry = chunksToUpdateLight.front();
    chunksToUpdateLight.pop();
    chunksInLightQueue.reset(entry.chunk->id);  // Clear bitset entry

    Chunk* chunk = entry.chunk;

    // Validate using direct ID lookup — O(1) instead of O(n) std::find
    if (chunk == nullptr || chunk->id >= OVERWORLD_SIZE_IN_CHUNKS) continue;
    if (chunks[chunk->id] != chunk) continue;  // Pointer mismatch = stale

    // Validate state (chunk may have been unloaded)
    if (!chunk->isLoaded()) continue;

    // Check if this chunk was upgraded to need full rebuild
    // This allows colorsOnly=true → colorsOnly=false upgrade without queue search
    bool actualColorsOnly = entry.colorsOnly && !chunksNeedingFullRebuild.test(chunk->id);
    
    // Clear the upgrade flag now that we're processing
    if (chunksNeedingFullRebuild.test(chunk->id)) {
      chunksNeedingFullRebuild.reset(chunk->id);
    }

    // For day/night color-only updates: skip chunks built very recently since
    // they already have correct light data from the build pass.
    // For block-change full rebuilds: always process immediately so the player
    // sees the geometry update within 1-2 ticks.
    if (actualColorsOnly) {
      if (g_ticksCounter >= chunk->loadedAtTick &&
          (g_ticksCounter - chunk->loadedAtTick) < 10)
        continue;

      u32 now;
      asm volatile("mfc0 %0, $9" : "=r"(now));
      if (processedColorsOnly >= MAX_COLORS_ONLY_PER_TICK ||
          (now - colorsOnlyStart) >= COLORS_ONLY_BUDGET_CYCLES) {
        // Defer the remaining day/night work to the next tick to smooth frame
        // time when many chunks are visible.
        if (!chunksInLightQueue.test(chunk->id)) {
          chunksToUpdateLight.push({chunk, actualColorsOnly});
          chunksInLightQueue.set(chunk->id);
        }
        break;
      }

      chunk->reloadLightColorsOnly();
      processedColorsOnly++;
    } else {
      if (processedFullRebuild >= MAX_FULL_REBUILDS_PER_TICK) {
        // Preserve ordering fairness while respecting per-tick cap.
        if (!chunksInLightQueue.test(chunk->id)) {
          chunksToUpdateLight.push({chunk, actualColorsOnly});
          chunksInLightQueue.set(chunk->id);
        }
        break;
      }
      chunk->reloadLightData();
      processedFullRebuild++;
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

// ===============================================================
//  Async light update with EditContext (NEW ENTRY POINT)
// ===============================================================

void ChunkManager::enqueueAffectedChunksForLightReload(
    const TyraCraft::EditContext& editCtx) {
  Chunk* editedChunk = getChunkByBlockOffset(editCtx.blockPos);

  // Fast path: opaque block edits without light emission only need the edited
  // chunk plus direct boundary neighbors (if the edited block is on a border).
  if (editCtx.isLightBlocking() && editCtx.oldLightValue == 0) {
    if (editedChunk && editedChunk->isLoaded()) {
      enqueueChunkToReloadLight(editedChunk, false);
    }

    static const Vec4 dirs[6] = {
        Vec4(0.0F, 1.0F, 0.0F), Vec4(0.0F, -1.0F, 0.0F),
        Vec4(1.0F, 0.0F, 0.0F), Vec4(-1.0F, 0.0F, 0.0F),
        Vec4(0.0F, 0.0F, 1.0F), Vec4(0.0F, 0.0F, -1.0F)};

    if (editedChunk) {
      for (u8 i = 0; i < 6; i++) {
        Vec4 neighbor = editCtx.blockPos + dirs[i];
        if (!editedChunk->containsBlock(&neighbor)) {
          Chunk* neighborChunk = getChunkByBlockOffset(neighbor);
          if (neighborChunk && neighborChunk->isLoaded()) {
            enqueueChunkToReloadLight(neighborChunk, false);
          }
        }
      }
    }

    return;
  }

  // Calculate radius dynamically based on light value
  float radiusInChunks = editCtx.getLightPropagationRadiusInChunks();

  std::vector<Chunk*> affectedChunks;
  getChunksInRadius(editCtx.blockPos, radiusInChunks, affectedChunks);

  // Keep local edits bounded: prioritize nearby chunks and restrict vertical
  // spread to adjacent chunk layers around the edited block.
  constexpr size_t MAX_AFFECTED_CHUNKS_PER_EDIT = 18;
  const float centerChunkY = editCtx.blockPos.y / CHUNK_SIZE;

  // Identify the chunk containing the edited block (needs full rebuild)

  std::sort(affectedChunks.begin(), affectedChunks.end(),
            [&editCtx](Chunk* a, Chunk* b) {
              if (!a) return false;
              if (!b) return true;

              const float da =
                  horizontalDistance2DSquared(editCtx.blockPos, a->center);
              const float db =
                  horizontalDistance2DSquared(editCtx.blockPos, b->center);
              return da < db;
            });

  size_t enqueued = 0;
  for (Chunk* chunk : affectedChunks) {
    if (!chunk || !chunk->isLoaded()) continue;

    const float chunkY = chunk->center.y / CHUNK_SIZE;
    if (fabsf(chunkY - centerChunkY) > 1.0f) continue;

    // Determine rebuild type based on chunk relationship to edited block:
    // 1. Edited chunk always needs full rebuild
    // 2. Immediate face-sharing neighbors need full rebuild if geometry changed
    // 3. Distant chunks only need light color updates
    bool colorsOnly = true;
    
    if (chunk == editedChunk) {
      // The chunk containing the edited block always needs full rebuild
      colorsOnly = false;
    } else if (editedChunk && editCtx.affectsGeometry) {
      // Check if this is an immediate neighbor (±1 chunk in X/Y/Z, but not diagonal)
      const int dx = abs((int)chunk->minOffset.x - (int)editedChunk->minOffset.x) / CHUNK_SIZE;
      const int dy = abs((int)chunk->minOffset.y - (int)editedChunk->minOffset.y) / CHUNK_SIZE;
      const int dz = abs((int)chunk->minOffset.z - (int)editedChunk->minOffset.z) / CHUNK_SIZE;
      const int totalOffset = dx + dy + dz;
      
      // Immediate face-sharing neighbor = exactly 1 chunk away in one direction
      if (totalOffset == 1) {
        colorsOnly = false;  // Needs full rebuild to show boundary faces
      }
    }
    
    enqueueChunkToReloadLight(chunk, colorsOnly);
    enqueued++;
    if (enqueued >= MAX_AFFECTED_CHUNKS_PER_EDIT) break;
  }
}

// ===============================================================
//  Legacy overload for backwards compatibility
// ===============================================================

void ChunkManager::enqueueAffectedChunksForLightReload(
    const Vec4& blockPos, float radiusInChunks, bool /*immediateUpdate*/) {
  // All chunks are enqueued for async processing — no synchronous BGM runs.
  // reloadLightDataAsync() uses a bounded batch + per-path caps/budgets
  // (colors-only throttled harder than full rebuild) to smooth frame times.
  std::vector<Chunk*> affectedChunks;
  getChunksInRadius(blockPos, radiusInChunks, affectedChunks);

  // Keep local edits bounded: prioritize nearby chunks and restrict vertical
  // spread to adjacent chunk layers around the edited block.
  constexpr size_t MAX_AFFECTED_CHUNKS_PER_EDIT = 18;
  const float centerChunkY = blockPos.y / CHUNK_SIZE;

  std::sort(affectedChunks.begin(), affectedChunks.end(),
            [&blockPos](Chunk* a, Chunk* b) {
              if (!a) return false;
              if (!b) return true;

              const float da =
                  horizontalDistance2DSquared(blockPos, a->center);
              const float db =
                  horizontalDistance2DSquared(blockPos, b->center);
              return da < db;
            });

  size_t enqueued = 0;
  for (Chunk* chunk : affectedChunks) {
    if (!chunk || !chunk->isLoaded()) continue;

    const float chunkY = chunk->center.y / CHUNK_SIZE;
    if (fabsf(chunkY - centerChunkY) > 1.0f) continue;

    enqueueChunkToReloadLight(chunk, false);  // block change = full rebuild
    enqueued++;
    if (enqueued >= MAX_AFFECTED_CHUNKS_PER_EDIT) break;
  }
}

// Needed to initiate light in all chunks. The visibleChunks will be available
// after the first update...
void ChunkManager::reloadLightDataOfAllChunks() {
  for (size_t i = 0; i < OVERWORLD_SIZE_IN_CHUNKS; i++) {
    if (chunks[i] && chunks[i]->isLoaded()) chunks[i]->reloadLightData();
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
  const int offsetZ =
      static_cast<int>((id - (offsetX * OVERWORLD_PAGE_IN_CHUNKS)) /
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
  Vec4 blockOffset = pLevel->worldPosToOffset(pos);
  Vec4 tempChunkMin =
      Vec4(std::floor(blockOffset.x / CHUNK_SIZE),
           std::floor(blockOffset.y / CHUNK_SIZE),
           std::floor(blockOffset.z / CHUNK_SIZE)) *
      CHUNK_SIZE;

  Chunk* chunk = getChunkByPosition(tempChunkMin);
  if (chunk) return chunk;

  const uint16_t id = getChunkIdByPosition(tempChunkMin);
  if (id >= OVERWORLD_SIZE_IN_CHUNKS) return nullptr;

  // On-demand path: if the renderer chunk does not exist yet, create it.
  return spawnChunk(id);
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

void ChunkManager::getColumnHeightInfo(int chunkX, int chunkZ, u8& outTopChunkY,
                                       bool& outHasBlocks) {
  const int clampedX = std::max(
      0,
      std::min(chunkX, static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1)));
  const int clampedZ = std::max(
      0,
      std::min(chunkZ, static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1)));
  const size_t gridIndex = clampedX * OVERWORLD_H_DISTANCE_IN_CHUNKS + clampedZ;

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
  const int minGridX =
      std::max(0, static_cast<int>(centerChunkX - radiusInChunks - 1));
  const int maxGridX =
      std::min(static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1),
               static_cast<int>(centerChunkX + radiusInChunks + 1));
  const int minGridZ =
      std::max(0, static_cast<int>(centerChunkZ - radiusInChunks - 1));
  const int maxGridZ =
      std::min(static_cast<int>(OVERWORLD_H_DISTANCE_IN_CHUNKS - 1),
               static_cast<int>(centerChunkZ + radiusInChunks + 1));

  // Squared radius for distance comparisons (avoids sqrt)
  const float radiusSquared = radiusInChunks * radiusInChunks;

  // Iterate only the grid cells within the bounding box
  for (int gridX = minGridX; gridX <= maxGridX; gridX++) {
    for (int gridZ = minGridZ; gridZ <= maxGridZ; gridZ++) {
      const size_t gridIndex = gridX * OVERWORLD_H_DISTANCE_IN_CHUNKS + gridZ;

      // On-demand renderer integration: ensure the whole vertical column exists
      // for this XZ before we query it.
      for (int yChunk = 0; yChunk < OVERWORLD_V_DISTANCE_IN_CHUNKS; yChunk++) {
        const uint16_t id = static_cast<uint16_t>(
            gridX * OVERWORLD_PAGE_IN_CHUNKS +
            gridZ * OVERWORLD_V_DISTANCE_IN_CHUNKS + yChunk);
        if (id < OVERWORLD_SIZE_IN_CHUNKS && chunks[id] == nullptr) {
          spawnChunk(id);
        }
      }

      // Process all vertical chunks in this XZ column
      for (Chunk* chunk : spatialGrid[gridIndex]) {
        // Validate chunk pointer
        if (chunk == nullptr) continue;

        // 2D distance check (XZ plane only, ignoring Y)
        const float distSquared =
            horizontalDistance2DSquared(center, chunk->center);
        const float distInChunksSquared =
            distSquared / (CHUNK_SIZE * CHUNK_SIZE);

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

  // Bounds check — world is [0, OVERWORLD_H_DISTANCE) × [0,
  // OVERWORLD_V_DISTANCE) × [0, OVERWORLD_H_DISTANCE)
  if (nx < 0 || nx >= OVERWORLD_H_DISTANCE || ny < 0 ||
      ny >= OVERWORLD_V_DISTANCE || nz < 0 || nz >= OVERWORLD_H_DISTANCE) {
    return nullptr;
  }

  Vec4 neighborOffset(nx, ny, nz);
  return getChunkByPosition(neighborOffset);
}

void ChunkManager::populateNeighborCache() {
  for (size_t i = 0; i < OVERWORLD_SIZE_IN_CHUNKS; i++) {
    Chunk* chunk = chunks[i];
    if (chunk == nullptr) continue;
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
                                             u8 maxRenderDistance,
                                             const float& deltaTime) {
  if (isVisibilityTaskRunning) {
    // Continue updating fade-in for ALL loaded chunks while background
    // task is running. This ensures newly built chunks start fading immediately
    // even if they aren't in the current visibleChunks list yet.
    for (size_t i = 0; i < loadedChunks.size(); i++) {
      loadedChunks[i]->update(frustumPlanes, deltaTime);
    }
    return;
  }

  Chunk* cameraChunk = getChunkByWorldPosition(*camPos);
  if (!cameraChunk) {
    update(frustumPlanes, camPos, maxRenderDistance, deltaTime);
    return;
  }

  // Capture current state for the background thread
  visTaskState.camPos.set(*camPos);
  visTaskState.camForward.set(camForward);
  visTaskState.maxRenderDistance = maxRenderDistance;
  for (int i = 0; i < 6; i++) {
    visTaskState.frustumPlanes[i] = frustumPlanes[i];
  }

  isVisibilityTaskRunning = true;

  auto* bgService = BackgroundTaskService::getInstance();
  visibilityTaskId = bgService->submit(
      [this, cameraChunk]() {
        // --- BACKGROUND WORKER (Thread-safe logic) ---
        nextVisibleChunks.clear();

        static std::bitset<OVERWORLD_SIZE_IN_CHUNKS> visited;
        static std::queue<VisBfsEntry> queue;
        visited.reset();
        while (!queue.empty()) queue.pop();

        // Root
        nextVisibleChunks.push_back(cameraChunk);
        visited.set(cameraChunk->id);

        // Find which faces are reachable from the camera's position within the
        // cameraChunk. This prevents seeing through walls if the player is
        // in a sealed hole.
        Vec4 blockOffset = pLevel->worldPosToOffset(visTaskState.camPos);
        int lx = static_cast<int>(blockOffset.x) -
                 static_cast<int>(cameraChunk->minOffset.x);
        int ly = static_cast<int>(blockOffset.y) -
                 static_cast<int>(cameraChunk->minOffset.y);
        int lz = static_cast<int>(blockOffset.z) -
                 static_cast<int>(cameraChunk->minOffset.z);

        // Clamp coordinates to handle near-edge positions and rounding errors
        lx = std::max(0, std::min(lx, CHUNK_SIZE - 1));
        ly = std::max(0, std::min(ly, CHUNK_SIZE - 1));
        lz = std::max(0, std::min(lz, CHUNK_SIZE - 1));

        u8 cameraVisibleFaces = GetVisibleFacesFromPosition(
            pLevel, (int)cameraChunk->minOffset.x, (int)cameraChunk->minOffset.y,
            (int)cameraChunk->minOffset.z, lx, ly, lz);

        const float directionalThreshold = -0.5f;

        for (u8 f = 0; f < FACE_COUNT; f++) {
          // Only traverse through faces reachable from the camera
          if (!(cameraVisibleFaces & (1 << f))) continue;

          Chunk* neighbor = cameraChunk->neighbors[f];
          if (neighbor && neighbor->isLoaded() && !visited.test(neighbor->id)) {
            queue.push({neighbor->id, OppositeFace(f), 1});
            visited.set(neighbor->id);
          }
        }

        while (!queue.empty()) {
          VisBfsEntry entry = queue.front();
          queue.pop();

          Chunk* chunk = chunks[entry.chunkId];
          if (!chunk || !chunk->isLoaded()) continue;

          if (entry.steps > visTaskState.maxRenderDistance) continue;

          // Frustum Check (using captured local planes)
          const auto frustumCheck = Utils::FrustumAABBIntersect(
              visTaskState.frustumPlanes, &chunk->scaledMinOffset,
              &chunk->scaledMaxOffset);
          if (frustumCheck == Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM) continue;

          // Directional filter
          float nx, ny, nz;
          GetFaceNormalVec(OppositeFace(entry.entryFace), nx, ny, nz);
          float dot = nx * visTaskState.camForward.x +
                      ny * visTaskState.camForward.y +
                      nz * visTaskState.camForward.z;
          if (dot < directionalThreshold) continue;

          nextVisibleChunks.push_back(chunk);

          for (u8 outFace = 0; outFace < FACE_COUNT; outFace++) {
            if (outFace == entry.entryFace) continue;
            if (chunk->isConnected(entry.entryFace, outFace)) {
              Chunk* neighbor = chunk->neighbors[outFace];
              if (neighbor && neighbor->isLoaded() && !visited.test(neighbor->id)) {
                if (entry.steps + 1 <= visTaskState.maxRenderDistance) {
                  queue.push({neighbor->id, OppositeFace(outFace),
                              (u8)(entry.steps + 1)});
                  visited.set(neighbor->id);
                }
              }
            }
          }
        }
      },
      [this]() {
        // --- MAIN THREAD COMPLETION (Final rendering state sync) ---
        visibleChunks.clear();
        for (Chunk* chunk : nextVisibleChunks) {
          // Update rendering state (frustum planes, fade) on main thread
          chunk->setCamPosition(&visTaskState.camPos);
          // Pass 0.0f as deltaTime here because the next frame's regular update
          // will handle the actual fade progression.
          chunk->update(visTaskState.frustumPlanes, 0.0f);
          visibleChunks.push_back(chunk);
        }
        isVisibilityTaskRunning = false;
      });
}

