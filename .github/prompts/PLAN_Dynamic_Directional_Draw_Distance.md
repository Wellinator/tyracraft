# Plan: Dynamic Directional Draw Distance with RAM-Adaptive Loading

**TL;DR**: Replace the current fixed-radius circular draw distance with a **dynamic, camera-direction-biased elliptical** loading system that self-regulates based on available RAM. The forward draw distance extends as far as memory allows (beyond the current 10-chunk max), while the backward distance is kept at ~50% of forward. Several complementary techniques — vertical column culling, occluded chunk unloading, empty chunk fast paths, and a third LOD tier — further reduce memory pressure to push the forward distance even further. The system monitors `get_used_memory()` against `MAX_SAFE_MEMORY_ALLOCATION` with a configurable safety margin and stops loading new chunks until old ones are freed.

**Steps**

## 1. RAM-Adaptive Draw Distance Controller

Create a new helper class `DrawDistanceController` in [inc/managers/draw_distance_controller.hpp](inc/managers/draw_distance_controller.hpp) / [src/managers/draw_distance_controller.cpp](src/managers/draw_distance_controller.cpp).

- Track `currentForwardDistance` (in chunks) and `currentBackwardDistance` (forward × 0.5).
- On each scheduling pass, query `get_used_memory()` and compare against `MAX_SAFE_MEMORY_ALLOCATION - SAFETY_MARGIN` (e.g., margin = 2 MB → threshold = 27 MB).
- If memory is **below threshold**: increment `currentForwardDistance` by 1 chunk (up to world bounds ~16).
- If memory is **above threshold**: decrement `currentForwardDistance` by 1 and prioritize unloading the farthest loaded chunks.
- Expose `getForwardDistance()`, `getBackwardDistance()`, and `canLoadMoreChunks()`.
- Remove the hard `MAX_DRAW_DISTANCE = 10` cap from [inc/constants.hpp](inc/constants.hpp#L58), replacing it with `MAX_DRAW_DISTANCE = 16` (world boundary). The actual limit becomes RAM-driven.

## 2. Directional (Elliptical) Chunk Scheduling

Modify `World::scheduleChunksNeighbors()` in [src/entities/World.cpp](src/entities/World.cpp#L379) to use an **asymmetric distance test** based on camera forward direction.

- Accept `Vec4 camForward` as an additional parameter (already available from `t_camera->unitCirclePosition` passed to `updateWithVisibilityGraph`).
- For each candidate chunk, compute the **signed projection** onto the camera forward vector:  
  $d_{proj} = \vec{(chunk - player)} \cdot \hat{camForward}$
- Compute the **perpendicular distance**:  
  $d_{perp} = \sqrt{d_{total}^2 - d_{proj}^2}$
- Apply an **elliptical test**:
  - Forward half ($d_{proj} \geq 0$): allow if $\left(\frac{d_{proj}}{D_{forward}}\right)^2 + \left(\frac{d_{perp}}{D_{side}}\right)^2 \leq 1$
  - Backward half ($d_{proj} < 0$): allow if $\left(\frac{|d_{proj}|}{D_{backward}}\right)^2 + \left(\frac{d_{perp}}{D_{side}}\right)^2 \leq 1$
- Where `D_forward` = `DrawDistanceController::getForwardDistance()`, `D_backward` = forward × 0.5, `D_side` = forward × 0.7 (or similar ratio).
- Modify `ChunkManager::getChunksInRadius()` in [src/managers/chunk_manager.cpp](src/managers/chunk_manager.cpp#L340) to accept an asymmetric bounding box instead of a symmetric radius, so the spatial grid query covers the elliptical region efficiently. Use `max(D_forward, D_side)` as the query radius, then filter with the elliptical test.

## 3. Priority-Based Load/Unload with Memory Gating

Modify the async dispatch system in `World`:

- In `addChunkToLoadAsync()`: before enqueuing, check `DrawDistanceController::canLoadMoreChunks()`. If over budget, skip the enqueue (the chunk stays `Clean`).
- In `loadScheduledChunks()` in [src/entities/World.cpp](src/entities/World.cpp#L486): add `get_used_memory()` check before calling `chunk->build()`. If memory is above the threshold, try to unload one chunk first (call `unloadScheduledChunks()` an extra time).
- In the sort comparator for `chunksToLoad`: prefer chunks with higher dot-product to camera forward (i.e., chunks more directly ahead load first).
- In unloading: prefer chunks with the **most negative** dot-product to camera forward (directly behind) for first eviction.

## 4. Unload Occluded Chunks (Existing TODO)

Currently, cave-culled chunks remain loaded in memory — their geometry is just not rendered. This wastes RAM. Implement the [TODO at docs/TODO.md](docs/TODO.md#L14):

- In `ChunkManager::updateWithVisibilityGraph()` at [src/managers/chunk_manager.cpp](src/managers/chunk_manager.cpp#L407), after the BFS finishes, iterate `loadedChunks` and identify chunks that were **not visited** by the BFS and are **not in frustum**.
- Track a per-chunk counter `consecutiveOccludedFrames`. Increment when occluded, reset when visible.
- If a chunk has been occluded for N consecutive frames (e.g., N = 60 ~2 seconds), schedule it for async unload via `addChunkToUnloadAsync()`.
- This frees geometry memory of interior/underground chunks that can't be seen, directly contributing to the RAM budget for extending forward distance.
- **Guard**: Keep at least 1-chunk "halo" around the camera always loaded (regardless of occlusion), to prevent popping when turning.

## 5. Vertical Column Culling (Skip Fully Underground Chunks)

The world is 8 chunks tall, but typically the top 2-3 contain surface/sky and the bottom 3-5 are entirely underground (solid or cave). Currently all 8 are loaded per XZ column.

- Build a **column heightmap** in `ChunkManager` at generation time: a `u8[16][16]` array storing the highest non-air Y-chunk index per XZ column. Update lazily when blocks change.
- During `scheduleChunksNeighbors()`, for each XZ column being loaded, apply a vertical filter:
  - Chunks **above** the heightmap with no blocks → skip loading (they're pure air, `build()` produces 0 vertices anyway).
  - Chunks **more than 2 levels below** the heightmap → mark as "deep underground" candidates. Only load them if the player's Y position is within 2 chunks vertically. Otherwise skip.
- This can skip 30-50% of chunks in a typical world, significantly reducing memory for the same XZ draw distance.

## 6. Empty Chunk Fast Path

In `Chunk::build()` at [src/entities/chunk.cpp](src/entities/chunk.cpp#L969), add a **pre-scan** before the 512-block iteration:

- Quick scan: iterate the `pLevel->map.blocks[]` range for this chunk's block offsets. If **all blocks are AIR** (or all are the same solid opaque type with no visible faces to outside), skip the full `buildNormaly()`/`buildCompressed()` pipeline entirely.
- Set `state = Loaded` with zero geometry. Mark a flag `isEmpty = true`.
- On `rebuildVisibilityGraph()`: empty chunks get `visibilityGraph = 0x7FFF` (all face pairs connected) for air, or `0x0000` for solid.
- This saves CPU time and avoids allocating geometry vectors for ~40-60% of chunks (sky and deep underground).

## 7. LOD Level 2 — Ultra-Distant Chunks

Currently there are only 2 LOD levels (0: full detail < 3 chunks, 1: greedy-meshed ≥ 3). Add a **LOD 2** for chunks at the extended forward distance:

- In `Chunk::getLODFromDistance()` at [src/entities/chunk.cpp](src/entities/chunk.cpp#L264):
  - Distance < 3: LOD 0 (full detail)
  - Distance 3-7: LOD 1 (compressed/greedy meshed)
  - Distance ≥ 8: LOD 2 (ultra-compressed)
- **LOD 2 build**: After greedy meshing, apply a second simplification pass — merge quads more aggressively by relaxing the `materialsMatch` color tolerance from 10 to 30, and merging across different texture UVs (use a uniform color or very low-res single-color atlas).
- Alternatively, for LOD 2, only emit **top-face and side-silhouette faces** — skip bottom faces and interior cavity faces entirely. This dramatically cuts vertex count for distant terrain.
- LOD 2 chunks use flat shading, low-res texture, and no alpha blending.

## 8. Camera Direction Smoothing

Rapid camera movement would cause constant load/unload thrashing. Add temporal smoothing:

- In `DrawDistanceController`, maintain a `smoothedForward` vector that blends toward the actual camera forward with a time constant (e.g., `lerp(smoothed, actual, 0.05)` per tick).
- Use `smoothedForward` for the elliptical scheduling test.
- Only re-run `scheduleChunksNeighbors()` when the smoothed direction has changed by more than ~15° (dot product < 0.96) OR the player has moved to a new chunk.

## 9. Update Constants and Config

- In [inc/constants.hpp](inc/constants.hpp#L57-L58): Change `MAX_DRAW_DISTANCE` to 16 (world limit), keep `MIN_DRAW_DISTANCE` at 2.
- Add new constants: `DRAW_DISTANCE_SAFETY_MARGIN_MB = 2`, `DRAW_DISTANCE_BACKWARD_RATIO = 0.5f`, `DRAW_DISTANCE_SIDE_RATIO = 0.7f`.
- In [inc/models/new_game_model.hpp](inc/models/new_game_model.hpp#L11): keep default `drawDistance = 4` as the **initial** forward distance (the controller will adapt upward from there).
- Keep the in-game menu slider functional — it sets the **minimum guaranteed** forward distance, the controller can only go above it.

## 10. Documentation Updates

- Update [docs/agents/chunk-system.md](docs/agents/chunk-system.md) with the new directional scheduling algorithm, LOD 2, and vertical culling.
- Update the constants table in [agents.md](agents.md) with new/changed constants.
- Update [docs/TODO.md](docs/TODO.md) to check off "Unload occluded chunks", "Memory usage", and "Increase draw distance".

---

## Verification

1. Enable `g_debug_menu.logChunkMemoryUsage` and monitor per-chunk memory in PCSX2 console output.
2. Monitor `get_used_memory()` in the debug overlay — verify it stays below `MAX_SAFE_MEMORY_ALLOCATION - 2MB` at all times.
3. Test directional loading: face a direction, verify forward chunks load further than backward. Turn 180° and verify the old forward chunks unload while new forward ones load (with smooth transition, no frame stall).
4. Test edge case: player at world corner facing outward — should not crash or over-query out-of-bounds grid cells.
5. Test vertical culling: in a flat terrain, verify underground chunks (Y=0,1,2) show as `Clean` state when player is on surface.
6. Test occluded chunk unloading: enter a cave, verify surface chunks above eventually unload when fully occluded for >2 seconds.
7. Test memory pressure: artificially lower `SAFETY_MARGIN` to observe the controller reducing draw distance.

---

## Decisions

- **Backward distance = 50% of forward**: Moderate reduction behind camera, avoids jarring pop-in on 180° turns while freeing ~25% of the circular area's memory for forward extension.
- **RAM-adaptive rather than fixed**: The draw distance controller dynamically adjusts based on `get_used_memory()`, so it works regardless of terrain complexity, game mode, or mob count.
- **Elliptical rather than cone**: An elliptical shape provides smooth transitions at the periphery (no sharp culling boundaries) and is simpler to compute than a true frustum-aligned shape.
- **Implementation order**: Steps 1-3 (directional + RAM-adaptive) are the core feature. Steps 4-7 are complementary techniques that increase the headroom the controller can exploit. Step 8 prevents thrashing. Each step can be implemented and tested independently.

---

## Key Files Reference

| What | File | Key Lines |
|---|---|---|
| Draw distance constants | [inc/constants.hpp](inc/constants.hpp) | L57-58 |
| Default draw distance | [inc/models/new_game_model.hpp](inc/models/new_game_model.hpp) | L11 |
| Chunk scheduling algorithm | [src/entities/World.cpp](src/entities/World.cpp) | L379-L476 |
| Distance calculation (2D XZ) | [inc/managers/chunk_manager.hpp](inc/managers/chunk_manager.hpp) | L78-L88 |
| Spatial grid radius query | [src/managers/chunk_manager.cpp](src/managers/chunk_manager.cpp) | L340-L380 |
| Visibility graph BFS (cave culling) | [src/managers/chunk_manager.cpp](src/managers/chunk_manager.cpp) | L407-L520 |
| Frustum AABB check | [src/entities/chunk.cpp](src/entities/chunk.cpp) | L1170-L1173 |
| LOD distance thresholds | [src/entities/chunk.cpp](src/entities/chunk.cpp) | L264-L272 |
| Chunk build pipeline | [src/entities/chunk.cpp](src/entities/chunk.cpp) | L969-L1035 |
| Greedy meshing | [src/entities/chunk.cpp](src/entities/chunk.cpp) | `compress()` / `mergeFaces()` |
| Set draw distance | [src/entities/World.cpp](src/entities/World.cpp) | L782-L817 |
| Chunk entity (full struct) | [inc/entities/chunk.hpp](inc/entities/chunk.hpp) | Full file |
| Level map arrays (3 MB) | [inc/entities/level.hpp](inc/entities/level.hpp) | L23-L31 |
| Memory limit constant | [inc/constants.hpp](inc/constants.hpp) | L226 |
| Visibility graph header | [inc/managers/visibility_graph.hpp](inc/managers/visibility_graph.hpp) | Full file |
| Chunk system doc | [docs/agents/chunk-system.md](docs/agents/chunk-system.md) | Full file |
