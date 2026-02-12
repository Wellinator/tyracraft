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

// Stack-allocated circular buffer queue for flood fill BFS
// Max capacity = CHUNK_LENGTH (512 for 8x8x8)
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

// Encode local coordinates (0-7 each) into a single u16 index
static inline u16 encodeLocal(int lx, int ly, int lz) {
  return (u16)((lx << 6) | (lz << 3) | ly);
}

// Decode local index back to coordinates
static inline void decodeLocal(u16 idx, int& lx, int& ly, int& lz) {
  ly = idx & 7;
  lz = (idx >> 3) & 7;
  lx = (idx >> 6) & 7;
}

// Bit manipulation for visited array (64 bytes = 512 bits)
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
         blockType == (u8)Blocks::TORCH;
}

u16 BuildVisibilityGraph(Level* pLevel, int chunkMinX, int chunkMinY,
                         int chunkMinZ) {
  u16 connectivity = 0;

  // Visited bitset: 512 bits = 64 bytes, one bit per block in chunk
  u8 visited[64];
  memset(visited, 0, sizeof(visited));

  // Static flood queue (on stack)
  FloodQueue queue;

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
