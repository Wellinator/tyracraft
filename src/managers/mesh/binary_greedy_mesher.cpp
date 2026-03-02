#include "managers/mesh/binary_greedy_mesher.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "entities/chunk.hpp"  // for TileGroup

const Tyra::Color BinaryGreedyMesher::kBaseColor = Tyra::Color(120.0f, 120.0f, 120.0f, 128.0f);

// ---------------------------------------------------------------------------
// Compile-time assertion: BGM bitmask type must cover CHUNK_SIZE bits.
// CHUNK_SIZE = 16 → u16 is sufficient (16 bits for 16 blocks per axis).
// ---------------------------------------------------------------------------
static_assert(CHUNK_SIZE <= 16, "CHUNK_SIZE > 16: BGM bitmask type (u16) is insufficient");

// ---------------------------------------------------------------------------
// isCuboidBlock
// Returns true for every block that the BGM should process.
// Special-shape blocks are excluded and must be handled by legacy builders.
// ---------------------------------------------------------------------------
bool BinaryGreedyMesher::isCuboidBlock(Blocks blockType) {
  switch (blockType) {
    // Air / void — never meshed
    case Blocks::VOID:
    case Blocks::AIR_BLOCK:
    // Vegetation (crossed)
    case Blocks::GRASS:
    case Blocks::POPPY_FLOWER:
    case Blocks::DANDELION_FLOWER:
    // Liquids — handled by liquid mesh builders
    case Blocks::WATER_BLOCK:
    case Blocks::LAVA_BLOCK:
    // Torch — custom shape
    case Blocks::TORCH:
    // Slabs — handled by processSlabs() in a separate BGM pass
    case Blocks::STONE_SLAB:
    case Blocks::BRICKS_SLAB:
    case Blocks::OAK_PLANKS_SLAB:
    case Blocks::SPRUCE_PLANKS_SLAB:
    case Blocks::BIRCH_PLANKS_SLAB:
    case Blocks::ACACIA_PLANKS_SLAB:
    case Blocks::STONE_BRICK_SLAB:
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      return false;
    default:
      return true;
  }
}

// ---------------------------------------------------------------------------
// isOpaqueNeighbour
// A face is visible when its neighbour is air, transparent, out-of-bounds, or
// void.  Returns true if the neighbour BLOCKS the face (i.e. is opaque solid
// or same-type transparent).
// ---------------------------------------------------------------------------
bool BinaryGreedyMesher::isOpaqueNeighbour(Level* pLevel,
                                           int nx, int ny, int nz,
                                           Blocks currentBlockType) {
  // Out-of-bounds → treat as transparent (expose the face)
  if (nx < 0 || ny < 0 || nz < 0 ||
      nx >= (int)pLevel->map.width  ||
      ny >= (int)pLevel->map.height ||
      nz >= (int)pLevel->map.length)
    return false;

  const u8 id = pLevel->GetBlockFromMap((uint16_t)nx, (uint16_t)ny, (uint16_t)nz);
  if (id <= (u8)Blocks::AIR_BLOCK) return false;

  const Blocks neighborType = static_cast<Blocks>(id);
  Block* neighborTpl = StaticBlockRepository::getInstance()->getBlockTemplate(neighborType);
  
  // Opaque blocks always block
  if (neighborTpl && !neighborTpl->hasTransparency()) return true;
  
  // Transparent blocks: hide shared faces between same-type blocks (e.g., glass-next-to-glass)
  if (neighborType == currentBlockType) return true;
  
  // Different transparent types don't occlude (show the face)
  return false;
}

// ---------------------------------------------------------------------------
// isFaceVisible
// ---------------------------------------------------------------------------
bool BinaryGreedyMesher::isFaceVisible(Level* pLevel,
                                       int bx, int by, int bz,
                                       FaceDir dir,
                                       Blocks currentBlockType) {
  int nx = bx, ny = by, nz = bz;
  switch (dir) {
    case FaceDir::PosX: nx = bx + 1; break;
    case FaceDir::NegX: nx = bx - 1; break;
    case FaceDir::PosY: ny = by + 1; break;
    case FaceDir::NegY: ny = by - 1; break;
    case FaceDir::PosZ: nz = bz + 1; break;
    case FaceDir::NegZ: nz = bz - 1; break;
  }
  return !isOpaqueNeighbour(pLevel, nx, ny, nz, currentBlockType);
}

// ---------------------------------------------------------------------------
// beginMeshChunk — initialise Output for incremental (per-direction) meshing.
// Call this once, then call processFaceDir() for each of the 6 directions,
// then call processSlabs().  The full meshChunk() combines all these steps.
// ---------------------------------------------------------------------------
void BinaryGreedyMesher::beginMeshChunk(Output& out) {
  out.opaqueVertices.clear();
  out.opaqueColors.clear();
  out.opaqueUV.clear();
  out.opaqueGroups.clear();
  out.transpVertices.clear();
  out.transpColors.clear();
  out.transpUV.clear();
  out.transpGroups.clear();

  out.opaqueVertices.reserve(384);
  out.opaqueColors.reserve(384);
  out.opaqueUV.reserve(384);
  out.transpVertices.reserve(64);
  out.transpColors.reserve(64);
  out.transpUV.reserve(64);
}

// ---------------------------------------------------------------------------
// meshChunk — full synchronous build (used by build() / reloadLightData())
// ---------------------------------------------------------------------------
void BinaryGreedyMesher::meshChunk(Level* pLevel,
                                   const Vec4& chunkMin,
                                   WorldLightModel* lightModel,
                                   Output& out) {
  beginMeshChunk(out);

  for (int d = 0; d < 6; ++d) {
    processFaceDir(pLevel, chunkMin, static_cast<FaceDir>(d), lightModel, out);
  }

  // Process slab blocks in a separate pass (individual quads, no greedy merge)
  processSlabs(pLevel, chunkMin, lightModel, out);
}

// ---------------------------------------------------------------------------
// Compute a per-face lit colour for a given face direction at block coords
// (bx, by, bz).  This mirrors CuboidMeshBuilder_loadLightData logic.
// ---------------------------------------------------------------------------
static Color computeFaceColor(Level* pLevel,
                              int bx, int by, int bz,
                              BinaryGreedyMesher::FaceDir dir,
                              float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0f;
  const float MIN_LIGHT_FACTOR = 0.15f;

  // Shading intensity per face direction (matches cuboid builder order)
  static constexpr float kIntensity[6] = {
      0.6f,  // PosX  RIGHT
      0.6f,  // NegX  LEFT
      1.0f,  // PosY  TOP
      0.5f,  // NegY  BOTTOM
      0.8f,  // PosZ  BACK
      0.8f,  // NegZ  FRONT
  };

  // Neighbour offset to sample light from
  static const int dx[6] = { 1, -1, 0,  0, 0,  0};
  static const int dy[6] = { 0,  0, 1, -1, 0,  0};
  static const int dz[6] = { 0,  0, 0,  0, 1, -1};

  const int d    = (int)dir;
  const int nx   = bx + dx[d];
  const int ny   = by + dy[d];
  const int nz   = bz + dz[d];

  u8 lightData = 0;
  if (nx >= 0 && ny >= 0 && nz >= 0 &&
      nx < (int)pLevel->map.width  &&
      ny < (int)pLevel->map.height &&
      nz < (int)pLevel->map.length) {
    lightData = pLevel->GetLightDataFromMap((uint16_t)nx, (uint16_t)ny, (uint16_t)nz);
  }

  const u8    sunLvl = (lightData >> 4) & 0xF;
  const u8    blkLvl =  lightData       & 0xF;

  const float sunFactor = std::max(
      (sunLvl * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);
  const float blkFactor = blkLvl / MAX_LIGHT_VALUE;
  const float factor    = std::max(sunFactor, blkFactor) * kIntensity[d];

  return Color(120.0f * factor, 120.0f * factor, 120.0f * factor, 128.0f);
}

// ---------------------------------------------------------------------------
// lightKey
// Pack the RGB channels of a lighting Color into a 24-bit key for
// equality comparison.  Alpha is always 128 (constant), so we skip it.
// Two faces can be greedy-merged only if they share the same lightKey.
// ---------------------------------------------------------------------------
static inline u32 lightKey(const Color& c) {
  return (static_cast<u32>((u8)c.r) << 16) |
         (static_cast<u32>((u8)c.g) <<  8) |
          static_cast<u32>((u8)c.b);
}

// ---------------------------------------------------------------------------
// processFaceDir
// Iterates all slices perpendicular to a face direction.  For each slice:
//
//   1. Build a 16×16 per-block light grid (lightGrid[row][col]).
//   2. For each visible face, group into a (texIdx, lightKey) bucket and
//      set the corresponding bit in that bucket's bitmask.
//   3. Run greedy rectangle merge within every bucket; emit one quad per
//      rectangle with the bucket's uniform light color.
//
// This prevents faces with different light levels from being merged into a
// single quad — a bug that produced repeating dark stripes across chunks.
// ---------------------------------------------------------------------------
void BinaryGreedyMesher::processFaceDir(Level* pLevel,
                                        const Vec4& chunkMin,
                                        FaceDir dir,
                                        WorldLightModel* lightModel,
                                        Output& out) {
  const int cs   = CHUNK_SIZE;
  const int minX = (int)chunkMin.x;
  const int minY = (int)chunkMin.y;
  const int minZ = (int)chunkMin.z;
  const float sunIntensity = lightModel ? lightModel->sunLightIntensity : 1.0f;

  // For each direction, we iterate over the "slice axis", with the two
  // "in-slice" axes being the "row axis" (A) and "column axis" (B).
  //
  // Mapping:
  //   PosX / NegX → slice = X, row = Z, col = Y
  //   PosY / NegY → slice = Y, row = X, col = Z
  //   PosZ / NegZ → slice = Z, row = X, col = Y

  for (int s = 0; s < cs; ++s) {
    // ------------------------------------------------------------------
    // Step 1: Compute the full per-block light grid for this slice.
    //
    // lightGrid[row][col] = lighting color for the face at (row, col)
    // within this slice.  This replaces the old "sample at cs/2" approach
    // that gave every block in a row the same (wrong) light value.
    // ------------------------------------------------------------------
    Color lightGrid[CHUNK_SIZE][CHUNK_SIZE];
    for (int row = 0; row < cs; ++row) {
      for (int col = 0; col < cs; ++col) {
        int lBx, lBy, lBz;
        switch (dir) {
          case FaceDir::PosX:
          case FaceDir::NegX:
            lBx = minX + s; lBz = minZ + row; lBy = minY + col; break;
          case FaceDir::PosY:
          case FaceDir::NegY:
            lBx = minX + row; lBy = minY + s; lBz = minZ + col; break;
          default: // PosZ / NegZ
            lBx = minX + row; lBy = minY + col; lBz = minZ + s; break;
        }
        lightGrid[row][col] = computeFaceColor(pLevel, lBx, lBy, lBz, dir, sunIntensity);
      }
    }

    // ------------------------------------------------------------------
    // Step 2: Iterate all blocks, group visible faces by (texIdx, lightKey).
    //
    // We store groups in a compact flat array (linear scan).  In practice
    // a 16³ chunk slice has very few distinct (tex, light) pairs — typically
    // 4–16 — so linear lookup is faster than a hash table on a PS2.
    //
    // Each group holds:
    //   texIdx, lightKey, lightColor   — identity
    //   masks[CHUNK_SIZE]              — greedy bitmask
    //   isTransparent                  — render stream selector
    //
    // Maximum theoretical groups per slice: 256 textures × many light keys.
    // We cap at 512 to bound stack usage; overflow falls back to merging
    // into an existing group with any matching texIdx (light accuracy loss
    // only under extreme conditions — millions of light values per 16² slice).
    // ------------------------------------------------------------------
    struct LightGroup {
      u8  texIdx;
      bool isTransparent;
      u32 lKey;            // packed RGB of lighting color
      Color lightColor;    // actual color to pass to emitQuad
      u16 masks[CHUNK_SIZE];
    };

    // Stack budget: 128 groups × ~50 bytes = ~6.4 KB — safe for PS2 stack.
    // Real chunks almost never exceed ~20-30 distinct (tex, light) pairs/slice.
    static const int kMaxGroups = 128;
    LightGroup groups[kMaxGroups];
    int numGroups = 0;

    for (int a = 0; a < cs; ++a) {
      for (int b = 0; b < cs; ++b) {
        // Map (s, a, b) → world block coords (bx, by, bz)
        int bx, by, bz;
        switch (dir) {
          case FaceDir::PosX:
          case FaceDir::NegX:
            bx = minX + s; bz = minZ + a; by = minY + b;
            break;
          case FaceDir::PosY:
          case FaceDir::NegY:
            by = minY + s; bx = minX + a; bz = minZ + b;
            break;
          default: // PosZ / NegZ
            bz = minZ + s; bx = minX + a; by = minY + b;
            break;
        }

        const u8 blockId = pLevel->GetBlockFromMap((uint16_t)bx, (uint16_t)by, (uint16_t)bz);
        if (blockId <= (u8)Blocks::AIR_BLOCK) continue;

        const Blocks bt = static_cast<Blocks>(blockId);
        if (!isCuboidBlock(bt)) continue;

        if (!isFaceVisible(pLevel, bx, by, bz, dir, bt)) continue;

        // Get texture index for this face from the block template
        Block*     tpl      = StaticBlockRepository::getInstance()->getBlockTemplate(bt);
        const auto faceMap  = tpl->getFacesMap();
        const bool isTransp = tpl->hasTransparency();

        // Map FaceDir → face map index (0=TOP,1=BOT,2=LEFT,3=RIGHT,4=BACK,5=FRONT)
        u8 faceMapIdx;
        switch (dir) {
          case FaceDir::PosY: faceMapIdx = 0; break;  // TOP
          case FaceDir::NegY: faceMapIdx = 1; break;  // BOTTOM
          case FaceDir::NegX: faceMapIdx = 2; break;  // LEFT  (-X)
          case FaceDir::PosX: faceMapIdx = 3; break;  // RIGHT (+X)
          case FaceDir::PosZ: faceMapIdx = 4; break;  // BACK
          default:            faceMapIdx = 5; break;  // FRONT (-Z)
        }
        const u8 texIdx = faceMap[faceMapIdx];

        // Per-block light from the precomputed grid
        const Color& faceColor = lightGrid[a][b];
        const u32    lk        = lightKey(faceColor);

        // Find or create a group for this (texIdx, lightKey) pair
        int gi = -1;
        for (int g = 0; g < numGroups; ++g) {
          if (groups[g].texIdx == texIdx && groups[g].lKey == lk) {
            gi = g;
            break;
          }
        }
        if (gi < 0) {
          if (numGroups < kMaxGroups) {
            gi = numGroups++;
            LightGroup& ng  = groups[gi];
            ng.texIdx        = texIdx;
            ng.isTransparent = isTransp;
            ng.lKey          = lk;
            ng.lightColor    = faceColor;
            for (int r = 0; r < cs; ++r) ng.masks[r] = 0;
          } else {
            // Overflow safety: fall back to any group with matching texIdx
            for (int g = 0; g < numGroups; ++g) {
              if (groups[g].texIdx == texIdx) { gi = g; break; }
            }
            if (gi < 0) gi = 0;  // give up, use first group — rare edge case
          }
        }

        // Set bit in the group's bitmask (col = b, row = a)
        groups[gi].masks[a] |= (u16)(1u << b);
      }
    }

    // ------------------------------------------------------------------
    // Step 3: Determine world-space base coords for this slice, then run
    // greedy rectangle scan on every (texIdx, lightKey) group.
    // ------------------------------------------------------------------
    int rowBase, colBase, sliceWorldCoord;
    switch (dir) {
      case FaceDir::PosX: case FaceDir::NegX:
        rowBase = minZ; colBase = minY; sliceWorldCoord = minX + s; break;
      case FaceDir::PosY: case FaceDir::NegY:
        rowBase = minX; colBase = minZ; sliceWorldCoord = minY + s; break;
      default:
        rowBase = minX; colBase = minY; sliceWorldCoord = minZ + s; break;
    }

    for (int g = 0; g < numGroups; ++g) {
      LightGroup& grp = groups[g];
      emitQuadsForSlice(grp.masks, sliceWorldCoord, rowBase, colBase,
                        dir, grp.texIdx, grp.lightColor, grp.isTransparent, out);
    }
  }
}

// ---------------------------------------------------------------------------

void BinaryGreedyMesher::emitQuadsForSlice(u16 masks[CHUNK_SIZE],
                                           int sliceCoord,
                                           int rowBase, int colBase,
                                           FaceDir dir, u8 texIndex,
                                           const Color& lightColor,
                                           bool isTransparent,
                                           Output& out) {
  const int cs = CHUNK_SIZE;

  for (int row = 0; row < cs; ++row) {
    u16 rowMask = masks[row];
#ifdef DEBUG_MODE
    int loopGuard = 0;
#endif
    while (rowMask) {
#ifdef DEBUG_MODE
      // Guard against non-progress loops in debug builds.
      if (++loopGuard > (cs * cs)) break;
#endif
      // Find the lowest set bit (first column of a span)
      const int col = __builtin_ctz(rowMask);

      // Find the width of continuous set bits from col
      // Build a mask of bits from col upward that are all set
      const u16 startBit = (u16)(1u << col);
      u16 spanMask = startBit;
      int width = 1;
      while ((col + width) < cs && (rowMask & (startBit << width))) {
        spanMask |= (u16)(startBit << width);
        ++width;
      }

      // Extend downward while each row also has all bits in spanMask set
      int height = 1;
      while ((row + height) < cs && (masks[row + height] & spanMask) == spanMask) {
        ++height;
      }

      // Claim the rectangle: clear spanMask bits in all affected rows
      for (int r = row; r < row + height; ++r) {
        masks[r] &= ~spanMask;
      }

      // Refresh the working mask for this row after clearing bits.
      rowMask = masks[row];

      // Emit one quad for this rectangle
      emitQuad(sliceCoord, rowBase + row, colBase + col,
               height, width,
               dir, texIndex, lightColor, isTransparent, out);
    }
  }
}

// ---------------------------------------------------------------------------
// emitQuad
// Builds 6 vertices (2 triangles, CCW winding) for a
// rectangular quad defined by its slice plane, row/col corner, and span dims.
//
// Coordinate mapping (world space = block offset × DOUBLE_BLOCK_SIZE):
//   DOUBLE_BLOCK_SIZE = 16.0f; block corners are at multiples of 16.
//
// Face vertices are computed in block-offset space (integers), then scaled
// to world space and packed as s16 (× 1 since DOUBLE_BLOCK_SIZE = 16 and
// s16 range = ±32767 → supports 2047 blocks → within world 128 blocks).
//
// UV coordinates tile the texture atlas: each block face occupies one
// kTile × kTile cell in the atlas.  Merged quads of size (width × height)
// tile the texture (width × height) times.
// ---------------------------------------------------------------------------
void BinaryGreedyMesher::emitQuad(int sliceCoord,
                                  int row, int col,
                                  int rowSpan, int colSpan,
                                  FaceDir dir, u8 texIndex,
                                  const Color& lightColor,
                                  bool isTransparent,
                                  Output& out) {
  // Atlas UV for tile (col, row within atlas)
  const float atlasCol = (texIndex % MAX_TEX_COLS);
  const float atlasRow = (texIndex / MAX_TEX_COLS);
  const float u0 = atlasCol * kTile;
  const float u1 = (atlasCol + 1.0f) * kTile;
  const float v0 = atlasRow * kTile;
  const float v1 = (atlasRow + 1.0f) * kTile;

  // For merged quads spanning multiple blocks the texture tiles.
  // We use the tile boundaries (u0..u1, v0..v1) and scale UVs so that
  // the full span covers exactly (rowSpan × colSpan) repetitions.
  // This matches the RegionRepeat wrap mode used in renderGrouped().

  const float uSpan = u1 - u0;
  const float vSpan = v1 - v0;

  // Unscaled UV extents across the merged span
  // (rowSpan tiles along the row axis, colSpan tiles along the col axis)
  const float uMax = u0 + uSpan * (float)rowSpan;
  const float vMax = v0 + vSpan * (float)colSpan;

  // Scale from block-offset space to world space (×DOUBLE_BLOCK_SIZE).
  // CRITICAL: Subtract BLOCK_SIZE to center blocks on the grid, matching
  // the old system's behavior where rawData vertices (-1 to +1) are scaled
  // by BLOCK_SIZE (8) then translated by offset×16, producing vertices from
  // (offset×16 - 8) to (offset×16 + 8).
  const float S = DOUBLE_BLOCK_SIZE;
  const float OFFSET = BLOCK_SIZE;  // 8.0 — half of DOUBLE_BLOCK_SIZE

  // Compute the 4 world-space corners of the quad.
  // Convention: the quad sits *on* the slice face — the slice coordinate
  // is adjusted by +1 for faces that face in the positive direction (so
  // the face is on the far side of the block).
  float x0, y0, z0, x1, y1, z1;

  switch (dir) {
    case FaceDir::PosX:
      // Face on +X side (right face).  Fix X = sliceCoord + 1.
      // Row axis = Z, Col axis = Y.
      x0 = (sliceCoord + 1) * S - OFFSET;  x1 = x0;
      z0 =  row              * S - OFFSET;  z1 = (row + rowSpan) * S - OFFSET;
      y0 =  col              * S - OFFSET;  y1 = (col + colSpan) * S - OFFSET;
      break;
    case FaceDir::NegX:
      // Left face, fix X = sliceCoord.
      x0 = sliceCoord * S - OFFSET;  x1 = x0;
      z0 =  row        * S - OFFSET;  z1 = (row + rowSpan) * S - OFFSET;
      y0 =  col        * S - OFFSET;  y1 = (col + colSpan) * S - OFFSET;
      break;
    case FaceDir::PosY:
      // Top face, fix Y = sliceCoord + 1.
      y0 = (sliceCoord + 1) * S - OFFSET;  y1 = y0;
      x0 =  row              * S - OFFSET;  x1 = (row + rowSpan) * S - OFFSET;
      z0 =  col              * S - OFFSET;  z1 = (col + colSpan) * S - OFFSET;
      break;
    case FaceDir::NegY:
      // Bottom face, fix Y = sliceCoord.
      y0 = sliceCoord * S - OFFSET;  y1 = y0;
      x0 =  row        * S - OFFSET;  x1 = (row + rowSpan) * S - OFFSET;
      z0 =  col        * S - OFFSET;  z1 = (col + colSpan) * S - OFFSET;
      break;
    case FaceDir::PosZ:
      // Back face, fix Z = sliceCoord + 1.
      z0 = (sliceCoord + 1) * S - OFFSET;  z1 = z0;
      x0 =  row              * S - OFFSET;  x1 = (row + rowSpan) * S - OFFSET;
      y0 =  col              * S - OFFSET;  y1 = (col + colSpan) * S - OFFSET;
      break;
    default: // FaceDir::NegZ — Front face
      z0 = sliceCoord * S - OFFSET;  z1 = z0;
      x0 =  row        * S - OFFSET;  x1 = (row + rowSpan) * S - OFFSET;
      y0 =  col        * S - OFFSET;  y1 = (col + colSpan) * S - OFFSET;
      break;
  }

  // Build the 4 corners as Vec4 positions.
  Vec4 corners[4];
  float uvCorners[4][2];  // [u, v] per corner

  // Corner convention: A=bottom-left, B=bottom-right, C=top-right, D=top-left
  // (in the 2D plane of the face).
  //
  //   PosX/NegX:  horizontal = Z, vertical = Y
  //   PosY/NegY:  horizontal = X, vertical = Z
  //   PosZ/NegZ:  horizontal = X, vertical = Y
  //
  // Triangle indices below MUST produce outward face normals under the BFC
  // convention used by ClippingManager:
  //   shouldBeBackfaceCulled(cam, v[2], v[1], v[0])
  //   normal = (v[1]-v[2]) x (v[0]-v[2])
  //
  // For Y/Z faces   {0,1,2  0,2,3} yields the correct outward normal.
  // For X faces the corners are laid out such that this ordering inverts the
  // normal; use {0,2,1  0,3,2} (reversed first vertex pair) to compensate.


  switch (dir) {
    case FaceDir::NegX: {
      // Normal points -X (LEFT face).  Winding: viewed from -X.
      // When looking at face from -X direction: +Y is up, +Z is right
      // CCW order: bottom-left, bottom-right, top-right, top-left
      corners[0] = Vec4(x0, y0, z0);  // A: bottom-front (y0, z0)
      corners[1] = Vec4(x0, y0, z1);  // B: bottom-back  (y0, z1)
      corners[2] = Vec4(x0, y1, z1);  // C: top-back     (y1, z1)
      corners[3] = Vec4(x0, y1, z0);  // D: top-front    (y1, z0)
      // V is inverted: bottom corners get vMax, top corners get v0
      uvCorners[0][0] = u0;   uvCorners[0][1] = vMax;
      uvCorners[1][0] = uMax; uvCorners[1][1] = vMax;
      uvCorners[2][0] = uMax; uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;   uvCorners[3][1] = v0;
      break;
    }
    case FaceDir::PosX: {
      // Normal +X (RIGHT face).  Winding: viewed from +X.
      // When looking at face from +X direction: +Y is up, -Z is right (mirror of NegX)
      // CCW order: bottom-back, bottom-front, top-front, top-back
      corners[0] = Vec4(x0, y0, z1);  // A: bottom-back  (y0, z1)
      corners[1] = Vec4(x0, y0, z0);  // B: bottom-front (y0, z0)
      corners[2] = Vec4(x0, y1, z0);  // C: top-front    (y1, z0)
      corners[3] = Vec4(x0, y1, z1);  // D: top-back     (y1, z1)
      // V is inverted: bottom corners get vMax, top corners get v0
      uvCorners[0][0] = u0;   uvCorners[0][1] = vMax;
      uvCorners[1][0] = uMax; uvCorners[1][1] = vMax;
      uvCorners[2][0] = uMax; uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;   uvCorners[3][1] = v0;
      break;
    }
    case FaceDir::PosY: {
      // Normal +Y (TOP face).  Winding: viewed from +Y looking down.
      // When looking at face from +Y direction: +X is right, +Z is down (away)
      // CCW order: front-left, front-right, back-right, back-left
      corners[0] = Vec4(x0, y0, z0);  // A: front-left  (x0, z0)
      corners[1] = Vec4(x1, y0, z0);  // B: front-right (x1, z0)
      corners[2] = Vec4(x1, y0, z1);  // C: back-right  (x1, z1)
      corners[3] = Vec4(x0, y0, z1);  // D: back-left   (x0, z1)
      uvCorners[0][0] = u0;   uvCorners[0][1] = v0;
      uvCorners[1][0] = uMax; uvCorners[1][1] = v0;
      uvCorners[2][0] = uMax; uvCorners[2][1] = vMax;
      uvCorners[3][0] = u0;   uvCorners[3][1] = vMax;
      break;
    }
    case FaceDir::NegY: {
      // Normal -Y (BOTTOM face).  Winding: viewed from -Y looking up.
      // When looking at face from -Y direction: +X is left, +Z is down (flipped vs TOP)
      // CCW order: back-left, back-right, front-right, front-left
      corners[0] = Vec4(x0, y0, z1);  // A: back-left   (x0, z1)
      corners[1] = Vec4(x1, y0, z1);  // B: back-right  (x1, z1)
      corners[2] = Vec4(x1, y0, z0);  // C: front-right (x1, z0)
      corners[3] = Vec4(x0, y0, z0);  // D: front-left  (x0, z0)
      uvCorners[0][0] = u0;   uvCorners[0][1] = v0;
      uvCorners[1][0] = uMax; uvCorners[1][1] = v0;
      uvCorners[2][0] = uMax; uvCorners[2][1] = vMax;
      uvCorners[3][0] = u0;   uvCorners[3][1] = vMax;
      break;
    }
    case FaceDir::PosZ: {
      // Normal +Z (BACK face).  Winding: viewed from +Z looking forward.
      // When looking at face from +Z direction: -X is right, +Y is up
      // CCW order: right-bottom, left-bottom, left-top, right-top
      corners[0] = Vec4(x1, y0, z0);  // A: right-bottom (x1, y0)
      corners[1] = Vec4(x0, y0, z0);  // B: left-bottom  (x0, y0)
      corners[2] = Vec4(x0, y1, z0);  // C: left-top     (x0, y1)
      corners[3] = Vec4(x1, y1, z0);  // D: right-top    (x1, y1)
      // V is inverted: bottom corners get vMax, top corners get v0
      uvCorners[0][0] = u0;   uvCorners[0][1] = vMax;
      uvCorners[1][0] = uMax; uvCorners[1][1] = vMax;
      uvCorners[2][0] = uMax; uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;   uvCorners[3][1] = v0;
      break;
    }
    default: { // NegZ — FRONT face
      // Normal -Z (FRONT face).  Winding: viewed from -Z looking back.
      // When looking at face from -Z direction: +X is left, +Y is up (mirror of BACK)
      // CCW order: left-bottom, right-bottom, right-top, left-top
      corners[0] = Vec4(x0, y0, z0);  // A: left-bottom  (x0, y0)
      corners[1] = Vec4(x1, y0, z0);  // B: right-bottom (x1, y0)
      corners[2] = Vec4(x1, y1, z0);  // C: right-top    (x1, y1)
      corners[3] = Vec4(x0, y1, z0);  // D: left-top     (x0, y1)
      // V is inverted: bottom corners get vMax, top corners get v0
      uvCorners[0][0] = u0;   uvCorners[0][1] = vMax;
      uvCorners[1][0] = uMax; uvCorners[1][1] = vMax;
      uvCorners[2][0] = uMax; uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;   uvCorners[3][1] = v0;
      break;
    }
  }

  // Triangle index sequences — see winding explanation in the comment above.
  // PosX / NegX require reversed winding so the software BFC check produces
  // an outward normal; all other directions use the standard sequence.
  int triIdx[6];
  if (dir == FaceDir::PosX || dir == FaceDir::NegX) {
    const int rev[6] = {0, 2, 1, 0, 3, 2};
    for (int i = 0; i < 6; ++i) triIdx[i] = rev[i];
  } else {
    const int fwd[6] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; ++i) triIdx[i] = fwd[i];
  }


  // Determine transparency — split opaque and transparent cuboid blocks into
  // separate output streams for correct rendering order. Legacy special blocks
  // (slabs, plants, liquids) are processed separately and appended post-BGM.
  const bool  bTransp    = isTransparent;
  auto&       outVerts   = bTransp ? out.transpVertices : out.opaqueVertices;
  auto&       outColors  = bTransp ? out.transpColors   : out.opaqueColors;
  auto&       outUV      = bTransp ? out.transpUV       : out.opaqueUV;
  auto&       groups     = bTransp ? out.transpGroups   : out.opaqueGroups;

  const u8  tileCol = texIndex % MAX_TEX_COLS;
  const u8  tileRow = texIndex / MAX_TEX_COLS;
  const u32 startIdx = (u32)outVerts.size();

  // Open or extend TileGroup
  if (groups.empty() ||
      groups.back().col != tileCol ||
      groups.back().row != tileRow) {
    TileGroup g;
    g.start = startIdx;
    g.count = 0;
    g.col   = tileCol;
    g.row   = tileRow;
    groups.push_back(g);
  }

  // Emit 6 vertices directly as Vec4/Color/Vec4
  for (int i = 0; i < 6; ++i) {
    const int ci = triIdx[i];
    outVerts.push_back(corners[ci]);
    outColors.push_back(lightColor);
    outUV.emplace_back(uvCorners[ci][0], uvCorners[ci][1], 1.0f, 0.0f);
  }

  groups.back().count += 6;
}

// ---------------------------------------------------------------------------
// isSlabBlock
// ---------------------------------------------------------------------------
bool BinaryGreedyMesher::isSlabBlock(Blocks blockType) {
  switch (blockType) {
    case Blocks::STONE_SLAB:
    case Blocks::BRICKS_SLAB:
    case Blocks::OAK_PLANKS_SLAB:
    case Blocks::SPRUCE_PLANKS_SLAB:
    case Blocks::BIRCH_PLANKS_SLAB:
    case Blocks::ACACIA_PLANKS_SLAB:
    case Blocks::STONE_BRICK_SLAB:
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      return true;
    default:
      return false;
  }
}

// ---------------------------------------------------------------------------
// processSlabs
// Iterates all blocks in the chunk, emitting individual half-height quads
// for each visible slab face directly into the BGM output.
// ---------------------------------------------------------------------------
void BinaryGreedyMesher::processSlabs(Level* pLevel,
                                       const Vec4& chunkMin,
                                       WorldLightModel* lightModel,
                                       Output& out) {
  const int cs   = CHUNK_SIZE;
  const int minX = (int)chunkMin.x;
  const int minY = (int)chunkMin.y;
  const int minZ = (int)chunkMin.z;

  for (int ly = 0; ly < cs; ++ly) {
    for (int lz = 0; lz < cs; ++lz) {
      for (int lx = 0; lx < cs; ++lx) {
        const int bx = minX + lx;
        const int by = minY + ly;
        const int bz = minZ + lz;

        const u8 blockId = pLevel->GetBlockFromMap((uint16_t)bx, (uint16_t)by, (uint16_t)bz);
        if (blockId <= (u8)Blocks::AIR_BLOCK) continue;

        const Blocks bt = static_cast<Blocks>(blockId);
        if (!isSlabBlock(bt)) continue;

        Block* tpl = StaticBlockRepository::getInstance()->getBlockTemplate(bt);
        const auto faceMap = tpl->getFacesMap();
        const bool isTransp = tpl->hasTransparency();
        const SlabOrientation orientation =
            pLevel->GetSlabOrientationDataFromMap((uint16_t)bx, (uint16_t)by, (uint16_t)bz);

        // Check each of 6 face directions
        static const FaceDir dirs[6] = {
            FaceDir::PosY, FaceDir::NegY,
            FaceDir::NegX, FaceDir::PosX,
            FaceDir::PosZ, FaceDir::NegZ
        };
        // FaceMap indices: 0=TOP, 1=BOTTOM, 2=LEFT, 3=RIGHT, 4=BACK, 5=FRONT
        static const u8 faceMapIdx[6] = {0, 1, 2, 3, 4, 5};

        for (int f = 0; f < 6; ++f) {
          if (!isFaceVisible(pLevel, bx, by, bz, dirs[f], bt)) continue;

          Color lightColor = computeFaceColor(pLevel, bx, by, bz, dirs[f],
                                              lightModel->sunLightIntensity);
          emitSlabQuad(bx, by, bz, dirs[f], faceMap[faceMapIdx[f]],
                       orientation, lightColor, isTransp, out);
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------
// emitSlabQuad
// Emits a single half-height slab face as 6 vertex entries.
// Side faces use half-height UVs (V spans 0.5 tile), top/bottom use full UVs.
// ---------------------------------------------------------------------------
void BinaryGreedyMesher::emitSlabQuad(int bx, int by, int bz,
                                       FaceDir dir, u8 texIndex,
                                       SlabOrientation orientation,
                                       const Color& lightColor,
                                       bool isTransparent,
                                       Output& out) {
  const float S = DOUBLE_BLOCK_SIZE;
  const float OFFSET = BLOCK_SIZE;
  const float halfS = S * 0.5f;

  // Compute Y bounds based on slab orientation
  float yLo, yHi;
  if (orientation == SlabOrientation::Bottom) {
    yLo = by * S - OFFSET;           // Bottom of block
    yHi = by * S - OFFSET + halfS;   // Halfway up
  } else {
    yLo = by * S - OFFSET + halfS;   // Halfway up
    yHi = by * S - OFFSET + S;       // Top of block
  }

  const float xLo = bx * S - OFFSET;
  const float xHi = bx * S - OFFSET + S;
  const float zLo = bz * S - OFFSET;
  const float zHi = bz * S - OFFSET + S;

  // Atlas UV for this tile
  const float atlasCol = (float)(texIndex % MAX_TEX_COLS);
  const float atlasRow = (float)(texIndex / MAX_TEX_COLS);
  const float u0 = atlasCol * kTile;
  const float u1 = (atlasCol + 1.0f) * kTile;
  const float v0 = atlasRow * kTile;
  const float v1 = (atlasRow + 1.0f) * kTile;
  const float vHalf = v0 + (v1 - v0) * 0.5f;  // Half-tile V for side faces

  Vec4 corners[4];
  float uvCorners[4][2];

  // Determine if this is a side face (needs half-height UV) or top/bottom (full UV)
  const bool isSideFace = (dir != FaceDir::PosY && dir != FaceDir::NegY);
  const float vEnd = isSideFace ? vHalf : v1;

  switch (dir) {
    case FaceDir::NegX: {
      corners[0] = Vec4(xLo, yLo, zLo);
      corners[1] = Vec4(xLo, yLo, zHi);
      corners[2] = Vec4(xLo, yHi, zHi);
      corners[3] = Vec4(xLo, yHi, zLo);
      // V is inverted: bottom corners get vEnd, top corners get v0
      uvCorners[0][0] = u0;  uvCorners[0][1] = vEnd;
      uvCorners[1][0] = u1;  uvCorners[1][1] = vEnd;
      uvCorners[2][0] = u1;  uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;  uvCorners[3][1] = v0;
      break;
    }
    case FaceDir::PosX: {
      corners[0] = Vec4(xHi, yLo, zHi);
      corners[1] = Vec4(xHi, yLo, zLo);
      corners[2] = Vec4(xHi, yHi, zLo);
      corners[3] = Vec4(xHi, yHi, zHi);
      // V is inverted: bottom corners get vEnd, top corners get v0
      uvCorners[0][0] = u0;  uvCorners[0][1] = vEnd;
      uvCorners[1][0] = u1;  uvCorners[1][1] = vEnd;
      uvCorners[2][0] = u1;  uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;  uvCorners[3][1] = v0;
      break;
    }
    case FaceDir::PosY: {
      corners[0] = Vec4(xLo, yHi, zLo);
      corners[1] = Vec4(xHi, yHi, zLo);
      corners[2] = Vec4(xHi, yHi, zHi);
      corners[3] = Vec4(xLo, yHi, zHi);
      uvCorners[0][0] = u0;  uvCorners[0][1] = v0;
      uvCorners[1][0] = u1;  uvCorners[1][1] = v0;
      uvCorners[2][0] = u1;  uvCorners[2][1] = v1;
      uvCorners[3][0] = u0;  uvCorners[3][1] = v1;
      break;
    }
    case FaceDir::NegY: {
      corners[0] = Vec4(xLo, yLo, zHi);
      corners[1] = Vec4(xHi, yLo, zHi);
      corners[2] = Vec4(xHi, yLo, zLo);
      corners[3] = Vec4(xLo, yLo, zLo);
      uvCorners[0][0] = u0;  uvCorners[0][1] = v0;
      uvCorners[1][0] = u1;  uvCorners[1][1] = v0;
      uvCorners[2][0] = u1;  uvCorners[2][1] = v1;
      uvCorners[3][0] = u0;  uvCorners[3][1] = v1;
      break;
    }
    case FaceDir::PosZ: {
      corners[0] = Vec4(xHi, yLo, zHi);
      corners[1] = Vec4(xLo, yLo, zHi);
      corners[2] = Vec4(xLo, yHi, zHi);
      corners[3] = Vec4(xHi, yHi, zHi);
      // V is inverted: bottom corners get vEnd, top corners get v0
      uvCorners[0][0] = u0;  uvCorners[0][1] = vEnd;
      uvCorners[1][0] = u1;  uvCorners[1][1] = vEnd;
      uvCorners[2][0] = u1;  uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;  uvCorners[3][1] = v0;
      break;
    }
    default: { // NegZ — FRONT face
      corners[0] = Vec4(xLo, yLo, zLo);
      corners[1] = Vec4(xHi, yLo, zLo);
      corners[2] = Vec4(xHi, yHi, zLo);
      corners[3] = Vec4(xLo, yHi, zLo);
      // V is inverted: bottom corners get vEnd, top corners get v0
      uvCorners[0][0] = u0;  uvCorners[0][1] = vEnd;
      uvCorners[1][0] = u1;  uvCorners[1][1] = vEnd;
      uvCorners[2][0] = u1;  uvCorners[2][1] = v0;
      uvCorners[3][0] = u0;  uvCorners[3][1] = v0;
      break;
    }
  }

  const int triIdx[6] = {0, 1, 2, 0, 2, 3};

  auto& outVerts  = isTransparent ? out.transpVertices : out.opaqueVertices;
  auto& outColors = isTransparent ? out.transpColors   : out.opaqueColors;
  auto& outUV     = isTransparent ? out.transpUV       : out.opaqueUV;
  auto& groups    = isTransparent ? out.transpGroups   : out.opaqueGroups;

  const u8  tileCol = texIndex % MAX_TEX_COLS;
  const u8  tileRow = texIndex / MAX_TEX_COLS;
  const u32 startIdx = (u32)outVerts.size();

  if (groups.empty() ||
      groups.back().col != tileCol ||
      groups.back().row != tileRow) {
    TileGroup g;
    g.start = startIdx;
    g.count = 0;
    g.col   = tileCol;
    g.row   = tileRow;
    groups.push_back(g);
  }

  for (int i = 0; i < 6; ++i) {
    const int ci = triIdx[i];
    outVerts.push_back(corners[ci]);
    outColors.push_back(lightColor);
    outUV.emplace_back(uvCorners[ci][0], uvCorners[ci][1], 1.0f, 0.0f);
  }

  groups.back().count += 6;
}
