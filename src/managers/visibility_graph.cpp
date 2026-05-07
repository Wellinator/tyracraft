/*
 * visibility_graph.cpp
 *
 * Implementation of the face-to-face visibility graph builder.
 * Uses flood fill through non-opaque blocks within a chunk to determine
 * which chunk faces can see each other.
 */

#include "managers/visibility_graph.hpp"
#include "entities/level.hpp"
#include <cstring>

// Bits needed per axis: log2(CHUNK_SIZE)
// CHUNK_SIZE=16 → 4 bits per axis, 12 bits total → max index 4095
// CHUNK_BITS and CHUNK_MASK are now defined globally in inc/constants.hpp
// (CLNUP-02, Phase 1) — file-local duplicates removed to keep a single
// source of truth.

// Stack-allocated circular buffer queue for flood fill BFS
// Max capacity = CHUNK_LENGTH (4096 for 16x16x16)
struct FloodQueue {
  u16 data[CHUNK_LENGTH];
  u16 head;
  u16 tail;
  u16 count;

  void reset() {
    head = 0;
    tail = 0;
    count = 0;
  }
  bool empty() const { return count == 0; }
  void push(u16 val) {
    data[tail] = val;
    tail = (tail + 1) % CHUNK_LENGTH;
    count++;
  }
  u16 pop() {
    u16 val = data[head];
    head = (head + 1) % CHUNK_LENGTH;
    count--;
    return val;
  }
};

// Encode local coordinates (0..CHUNK_SIZE-1 each) into a single u16 index
static inline u16 encodeLocal(int lx, int ly, int lz) {
  return (u16)((lx << (CHUNK_BITS * 2)) | (lz << CHUNK_BITS) | ly);
}

// Decode local index back to coordinates
static inline void decodeLocal(u16 idx, int& lx, int& ly, int& lz) {
  ly = idx & CHUNK_MASK;
  lz = (idx >> CHUNK_BITS) & CHUNK_MASK;
  lx = (idx >> (CHUNK_BITS * 2)) & CHUNK_MASK;
}

// Bit manipulation for visited array (CHUNK_LENGTH/8 bytes = CHUNK_LENGTH bits)
static inline bool isVisited(const u8* visited, u16 idx) {
  return (visited[idx >> 3] >> (idx & 7)) & 1;
}

static inline void setVisited(u8* visited, u16 idx) {
  visited[idx >> 3] |= (1 << (idx & 7));
}

bool IsBlockTransparentForFlood(u8 blockType) {
  // A block is transparent for flood fill if it's NOT opaque.
  // This matches the inverse of CuboidMeshBuilder_isBlockOpaque.
  return blockType == (u8)Blocks::VOID ||
         blockType == (u8)Blocks::AIR_BLOCK ||
         blockType == (u8)Blocks::GLASS_BLOCK ||
         blockType == (u8)Blocks::POPPY_FLOWER ||
         blockType == (u8)Blocks::DANDELION_FLOWER ||
         blockType == (u8)Blocks::GRASS ||
         blockType == (u8)Blocks::WATER_BLOCK ||
         blockType == (u8)Blocks::LAVA_BLOCK ||
         blockType == (u8)Blocks::TORCH ||
         blockType == (u8)Blocks::OAK_LEAVES_BLOCK ||
         blockType == (u8)Blocks::BIRCH_LEAVES_BLOCK;
}

u16 BuildVisibilityGraph(Level* pLevel, int chunkMinX, int chunkMinY,
                         int chunkMinZ) {
  u16 connectivity = 0;

  // Visited bitset: CHUNK_LENGTH bits = CHUNK_LENGTH/8 bytes, one bit per block
  // Static to avoid ~512 bytes stack allocation each call (PS2 stack is limited)
  static u8 visited[CHUNK_LENGTH / 8];
  memset(visited, 0, sizeof(visited));

  // Static flood queue to avoid ~8KB stack allocation (PS2 stack is limited)
  static FloodQueue queue;

  // 6 neighbor offsets in local space: +x, -x, +z, -z, +y, -y
  static const int dx[6] = {1, -1, 0, 0, 0, 0};
  static const int dy[6] = {0, 0, 0, 0, 1, -1};
  static const int dz[6] = {0, 0, 1, -1, 0, 0};

  // For each block in the chunk
  for (int lx = 0; lx < CHUNK_SIZE; lx++) {
    for (int lz = 0; lz < CHUNK_SIZE; lz++) {
      for (int ly = 0; ly < CHUNK_SIZE; ly++) {
        u16 startIdx = encodeLocal(lx, ly, lz);

        // Skip if already visited
        if (isVisited(visited, startIdx)) continue;

        // Skip if this block is opaque
        int wx = chunkMinX + lx;
        int wy = chunkMinY + ly;
        int wz = chunkMinZ + lz;
        u8 blockType = pLevel->GetBlockFromMap(wx, wy, wz);
        if (!IsBlockTransparentForFlood(blockType)) {
          setVisited(visited, startIdx);
          continue;
        }

        // BFS flood fill from this block
        u8 touchedFaces = 0;  // 6-bit mask of faces touched by this flood
        queue.reset();
        queue.push(startIdx);
        setVisited(visited, startIdx);

        while (!queue.empty()) {
          u16 curIdx = queue.pop();
          int cx, cy, cz;
          decodeLocal(curIdx, cx, cy, cz);

          // Check all 6 neighbors
          for (int dir = 0; dir < 6; dir++) {
            int nx = cx + dx[dir];
            int ny = cy + dy[dir];
            int nz = cz + dz[dir];

            // Check if neighbor is outside chunk boundary
            if (nx < 0) {
              touchedFaces |= (1 << FACE_SOUTH);
              continue;
            }
            if (nx >= CHUNK_SIZE) {
              touchedFaces |= (1 << FACE_NORTH);
              continue;
            }
            if (nz < 0) {
              touchedFaces |= (1 << FACE_WEST);
              continue;
            }
            if (nz >= CHUNK_SIZE) {
              touchedFaces |= (1 << FACE_EAST);
              continue;
            }
            if (ny < 0) {
              touchedFaces |= (1 << FACE_BOTTOM);
              continue;
            }
            if (ny >= CHUNK_SIZE) {
              touchedFaces |= (1 << FACE_TOP);
              continue;
            }

            u16 nIdx = encodeLocal(nx, ny, nz);
            if (isVisited(visited, nIdx)) continue;

            // Check if neighbor block is transparent
            int nwx = chunkMinX + nx;
            int nwy = chunkMinY + ny;
            int nwz = chunkMinZ + nz;
            u8 nBlockType = pLevel->GetBlockFromMap(nwx, nwy, nwz);
            if (!IsBlockTransparentForFlood(nBlockType)) {
              setVisited(visited, nIdx);
              continue;
            }

            setVisited(visited, nIdx);
            queue.push(nIdx);
          }

          // Early termination: if all 6 faces are touched, max connectivity
          if (touchedFaces == 0x3F) {
            // Mark remaining blocks in this flood as visited (drain queue)
            while (!queue.empty()) {
              u16 remaining = queue.pop();
              // Already visited when pushed, nothing more needed
              (void)remaining;
            }
            break;
          }
        }

        // Convert touchedFaces bitmask to face-pair connectivity
        // For every pair of faces that were both touched, set their bit
        for (u8 a = 0; a < FACE_COUNT; a++) {
          if (!(touchedFaces & (1 << a))) continue;
          for (u8 b = a + 1; b < FACE_COUNT; b++) {
            if (touchedFaces & (1 << b)) {
              SetConnected(connectivity, a, b);
            }
          }
        }

        // If we already have full connectivity, no need to continue
        if (connectivity == VIS_GRAPH_ALL_CONNECTED) {
          return connectivity;
        }
      }
    }
  }

  return connectivity;
}
u8 GetVisibleFacesFromPosition(Level* pLevel, int chunkMinX, int chunkMinY,
                               int chunkMinZ, int lx, int ly, int lz) {
  // Coordinates are clamped by the caller, but we verify here for safety.
  if (lx < 0 || lx >= CHUNK_SIZE || ly < 0 || ly >= CHUNK_SIZE || lz < 0 ||
      lz >= CHUNK_SIZE) {
    return 0x3F;
  }

  // Check if starting block is opaque
  u8 startBlock = pLevel->GetBlockFromMap(chunkMinX + lx, chunkMinY + ly,
                                          chunkMinZ + lz);
  if (!IsBlockTransparentForFlood(startBlock)) {
    // If the camera is inside an opaque block (wall), we can't see any
    // faces of the chunk. This prevents the "flash" of visibility when 
    // touching a wall.
    return 0;
  }

  static u8 visited[CHUNK_LENGTH / 8];
  memset(visited, 0, sizeof(visited));
  static FloodQueue queue;
  queue.reset();

  u16 startIdx = encodeLocal(lx, ly, lz);
  queue.push(startIdx);
  setVisited(visited, startIdx);

  u8 touchedFaces = 0;
// ... (rest of function remains same)

  static const int dx[6] = {1, -1, 0, 0, 0, 0};
  static const int dy[6] = {0, 0, 0, 0, 1, -1};
  static const int dz[6] = {0, 0, 1, -1, 0, 0};

  while (!queue.empty()) {
    u16 curIdx = queue.pop();
    int cx, cy, cz;
    decodeLocal(curIdx, cx, cy, cz);

    for (int dir = 0; dir < 6; dir++) {
      int nx = cx + dx[dir];
      int ny = cy + dy[dir];
      int nz = cz + dz[dir];

      if (nx < 0) {
        touchedFaces |= (1 << FACE_SOUTH);
        continue;
      }
      if (nx >= CHUNK_SIZE) {
        touchedFaces |= (1 << FACE_NORTH);
        continue;
      }
      if (nz < 0) {
        touchedFaces |= (1 << FACE_WEST);
        continue;
      }
      if (nz >= CHUNK_SIZE) {
        touchedFaces |= (1 << FACE_EAST);
        continue;
      }
      if (ny < 0) {
        touchedFaces |= (1 << FACE_BOTTOM);
        continue;
      }
      if (ny >= CHUNK_SIZE) {
        touchedFaces |= (1 << FACE_TOP);
        continue;
      }

      u16 nIdx = encodeLocal(nx, ny, nz);
      if (isVisited(visited, nIdx)) continue;

      u8 nBlockType = pLevel->GetBlockFromMap(chunkMinX + nx, chunkMinY + ny,
                                              chunkMinZ + nz);
      if (!IsBlockTransparentForFlood(nBlockType)) {
        setVisited(visited, nIdx);
        continue;
      }

      setVisited(visited, nIdx);
      queue.push(nIdx);
    }

    if (touchedFaces == 0x3F) break;
  }

  return touchedFaces;
}
