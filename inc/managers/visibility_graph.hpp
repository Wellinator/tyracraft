/*
 * visibility_graph.hpp
 *
 * Face-to-face visibility graph for cave culling.
 * Each chunk stores a 15-bit connectivity bitmask representing which pairs
 * of chunk faces can "see" each other through non-opaque blocks.
 *
 * Based on Tomcc's algorithm from Minecraft PE:
 * https://tomcc.github.io/2014/08/31/visibility-1.html
 */

#pragma once

#include <tamtypes.h>
#include "constants.hpp"

// Forward declarations
class Level;

// The 6 faces of a chunk, used as indices into the visibility graph.
enum ChunkFace : u8 {
  FACE_NORTH = 0,   // +X direction
  FACE_SOUTH = 1,   // -X direction
  FACE_EAST = 2,    // +Z direction
  FACE_WEST = 3,    // -Z direction
  FACE_TOP = 4,     // +Y direction
  FACE_BOTTOM = 5,  // -Y direction
  FACE_COUNT = 6
};

// Total number of unique face pairs: C(6,2) = 15
#define VIS_GRAPH_PAIR_COUNT 15

// All 6 faces connected to each other (fully connected chunk, e.g. all air)
#define VIS_GRAPH_ALL_CONNECTED 0x7FFF

/**
 * @brief Get the bit index for a face pair (a, b) in the 15-bit graph.
 * Uses triangular indexing: for a < b, index = a*(2*N-a-1)/2 + (b-a-1)
 * where N = 6 (number of faces).
 */
inline int GetFacePairIndex(u8 a, u8 b) {
  if (a > b) {
    u8 tmp = a;
    a = b;
    b = tmp;
  }
  // Triangular number formula for upper triangle of 6x6 matrix
  return (a * (2 * FACE_COUNT - a - 1)) / 2 + (b - a - 1);
}

/**
 * @brief Test if two faces are connected in the visibility graph.
 */
inline bool IsConnected(u16 graph, u8 a, u8 b) {
  if (a == b) return true;  // Same face is always connected to itself
  return (graph >> GetFacePairIndex(a, b)) & 1;
}

/**
 * @brief Set the connectivity bit for a face pair.
 */
inline void SetConnected(u16& graph, u8 a, u8 b) {
  if (a == b) return;
  graph |= (1 << GetFacePairIndex(a, b));
}

/**
 * @brief Get the opposite face.
 */
inline u8 OppositeFace(u8 f) {
  // NORTH<->SOUTH, EAST<->WEST, TOP<->BOTTOM
  // 0<->1, 2<->3, 4<->5
  return f ^ 1;
}

/**
 * @brief Get the face direction offsets for neighbor chunk lookup.
 * Returns the block-space offset to add to minOffset to reach the neighbor.
 * @param face The chunk face direction
 * @param dx Output X offset (in blocks, multiple of CHUNK_SIZE)
 * @param dy Output Y offset
 * @param dz Output Z offset
 */
inline void GetFaceDirection(u8 face, int& dx, int& dy, int& dz) {
  dx = dy = dz = 0;
  switch (face) {
    case FACE_NORTH:
      dx = CHUNK_SIZE;
      break;  // +X
    case FACE_SOUTH:
      dx = -CHUNK_SIZE;
      break;  // -X
    case FACE_EAST:
      dz = CHUNK_SIZE;
      break;  // +Z
    case FACE_WEST:
      dz = -CHUNK_SIZE;
      break;  // -Z
    case FACE_TOP:
      dy = CHUNK_SIZE;
      break;  // +Y
    case FACE_BOTTOM:
      dy = -CHUNK_SIZE;
      break;  // -Y
  }
}

/**
 * @brief Build the visibility graph for the chunk at the given block-space
 * origin. Uses flood fill through non-opaque blocks to determine which chunk
 * faces can see each other.
 *
 * @param pLevel  Pointer to the Level for block data access
 * @param chunkMinX  X coordinate of chunk origin (in block coords)
 * @param chunkMinY  Y coordinate of chunk origin (in block coords)
 * @param chunkMinZ  Z coordinate of chunk origin (in block coords)
 * @return 15-bit connectivity bitmask
 */
u16 BuildVisibilityGraph(Level* pLevel, int chunkMinX, int chunkMinY,
                         int chunkMinZ);

/**
 * @brief Check if a block type is transparent for flood fill purposes.
 * Transparent blocks allow visibility to pass through them.
 * This is the inverse of the opaque check used in mesh builders.
 */
bool IsBlockTransparentForFlood(u8 blockType);
