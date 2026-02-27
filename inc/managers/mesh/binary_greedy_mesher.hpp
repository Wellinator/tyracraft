#pragma once

#include <vector>
#include <array>
#include <tamtypes.h>
#include <math/vec4.hpp>
#include "entities/Block.hpp"
#include "entities/level.hpp"
#include "models/world_light_model.hpp"
#include "constants.hpp"

using Tyra::Color;
using Tyra::Vec4;

// Forward declaration
struct TileGroup;

/**
 * @brief Binary Greedy Meshing (BGM) for cuboid blocks within a chunk.
 *
 * Replaces the old per-block MeshBuilder dispatch + two-pass mergeFaces() with
 * a single bitmask-based algorithm that finds maximal rectangles of identical
 * faces in O(n) bitwise operations.
 *
 * Operation:
 *   For each of the 6 face directions, iterate slices perpendicular to that
 *   axis.  For each slice, build a u16 bitmask per (texture_index) — bit=1
 *   where that face is visible AND the block is a cuboid with that texture.
 *   Use bitwise row-scan to find maximal-width spans, then extend vertically
 *   while subsequent rows contain the same span.  Emit one quad per such
 *   rectangle.
 *
 * Chunk dimensions: CHUNK_SIZE × CHUNK_SIZE × CHUNK_SIZE (currently 16³).
 * Slice bitmask width = CHUNK_SIZE bits → fits in a u16 (up to 16).
 *
 * Special blocks (GRASS flower, POPPY, DANDELION, TORCH, WATER, LAVA)
 * are excluded from the BGM and must be processed separately by the
 * existing MeshBuilder dispatch.  Slabs are handled by a dedicated
 * second pass within meshChunk() (processSlabs).
 *
 * Output vertices are stored directly as Vec4/Color/Vec4 arrays, ready for
 * immediate rendering without any decompression step.  TileGroup boundaries
 * are built in parallel so renderGrouped() can use RegionRepeat tiling.
 */
class BinaryGreedyMesher {
 public:
  BinaryGreedyMesher() = default;
  ~BinaryGreedyMesher() = default;

  struct Output {
    std::vector<Vec4>       opaqueVertices;
    std::vector<Color>      opaqueColors;
    std::vector<Vec4>       opaqueUV;
    std::vector<TileGroup>  opaqueGroups;
    std::vector<Vec4>       transpVertices;
    std::vector<Color>      transpColors;
    std::vector<Vec4>       transpUV;
    std::vector<TileGroup>  transpGroups;
  };

  /**
   * Build all cuboid-block geometry for the given chunk region.
   *
   * @param pLevel      World data (block types + light data)
   * @param chunkMin    Minimum block-offset corner (inclusive)
   * @param lightModel  World light model for sun intensity
   * @param out         Result — populated by this call (cleared first)
   *
   * Blocks that are not "pure cuboids" are skipped; the caller is responsible
   * for processing them via the legacy MeshBuilder dispatch and appending to
   * the same output vectors and appending directly.
   */
  void meshChunk(Level* pLevel,
                 const Vec4& chunkMin,
                 WorldLightModel* lightModel,
                 Output& out);

  /**
   * Test whether a block type should be processed by the BGM greedy pass
   * (i.e. it is a simple cuboid block, not a special shape).
   */
  static bool isCuboidBlock(Blocks blockType);

  /** Test whether a block type is a slab (processed by the BGM slab pass). */
  static bool isSlabBlock(Blocks blockType);

 public:
  /** Face direction enum — maps to axis + sign. */
  enum class FaceDir : u8 {
    PosX = 0,  // RIGHT face (+X neighbour must be transparent)
    NegX = 1,  // LEFT  face (-X neighbour)
    PosY = 2,  // TOP   face (+Y neighbour)
    NegY = 3,  // BOTTOM face (-Y neighbour)
    PosZ = 4,  // BACK  face (+Z neighbour)
    NegZ = 5,  // FRONT face (-Z neighbour)
  };

 private:
  // -------------------------------------------------------------------------
  // Internal helpers
  // -------------------------------------------------------------------------

  /**
   * Emit all quads for one face direction across the entire chunk.
   * For each slice, builds a per-block 2D light grid and groups visible faces
   * by (texIdx, lightKey) to prevent merging blocks with different light levels.
   */
  void processFaceDir(Level* pLevel,
                      const Vec4& chunkMin,
                      FaceDir dir,
                      WorldLightModel* lightModel,
                      Output& out);

  /**
   * Greedy rectangle scan with a uniform light color.
   * Given a flat bitmask array (CHUNK_SIZE rows, each a u16), find all maximal
   * rectangles via greedy scan and emit one quad per rectangle.
   *
   * Called once per (texIdx, lightKey) group \u2014 all faces in a group share the
   * same texture and identical lighting, so the uniform color is correct.
   *
   * @param masks         Row bitmasks.  Modified in-place (bits cleared as claimed).
   * @param sliceCoord    World-block coordinate of the slice plane.
   * @param rowBase       World-block coordinate of row 0.
   * @param colBase       World-block coordinate of col 0.
   * @param dir           Face direction.
   * @param texIndex      Atlas texture index.
   * @param lightColor    Uniform light color for all quads in this call.
   * @param isTransparent True if this texture comes from transparent blocks.
   * @param out           Output buffer.
   */
  void emitQuadsForSlice(u16 masks[CHUNK_SIZE],
                         int sliceCoord, int rowBase, int colBase,
                         FaceDir dir, u8 texIndex,
                         const Color& lightColor,
                         bool isTransparent,
                         Output& out);

  /** Emit a single quad (2 triangles = 6 vertices) into the output.
   *  @param isTransparent True if this quad comes from a transparent block. */
  void emitQuad(int sliceCoord, int row, int col,
                int rowSpan, int colSpan,
                FaceDir dir, u8 texIndex,
                const Color& lightColor,
                bool isTransparent,
                Output& out);

  /** Process all slab blocks in the chunk, emitting individual quads
   *  (no greedy merge) directly into the BGM output. */
  void processSlabs(Level* pLevel,
                    const Vec4& chunkMin,
                    WorldLightModel* lightModel,
                    Output& out);

  /** Emit a single slab face quad (half-height cuboid). */
  void emitSlabQuad(int bx, int by, int bz,
                    FaceDir dir, u8 texIndex,
                    SlabOrientation orientation,
                    const Color& lightColor,
                    bool isTransparent,
                    Output& out);

  /** Check if the face of a block in direction @p dir is visible
   *  (neighbour is air/transparent or is a void/boundary, but not same-type transparent).
   *  @param currentBlockType The block type being checked (for same-type culling). */
  static bool isFaceVisible(Level* pLevel, int bx, int by, int bz,
                            FaceDir dir, Blocks currentBlockType);

  /** Check if a block is opaque (blocks faces adjacent to it).
   *  Same-type transparent blocks are treated as opaque (hide shared faces).
   *  @param currentBlockType The block type being checked (for same-type culling). */
  static bool isOpaqueNeighbour(Level* pLevel, int nx, int ny, int nz,
                                Blocks currentBlockType);

  // Face shading intensities — same values as CuboidMeshBuilder_loadLightData.
  // Order: PosX, NegX, PosY, NegY, PosZ, NegZ
  static constexpr float kFaceIntensity[6] = {
      0.6f,  // RIGHT  (+X)
      0.6f,  // LEFT   (-X)
      1.0f,  // TOP    (+Y)
      0.5f,  // BOTTOM (-Y)
      0.8f,  // BACK   (+Z)
      0.8f,  // FRONT  (-Z)
  };

  // Base block colour (same as cuboid builder)
  static const Tyra::Color kBaseColor;

  // UV tile size in normalised atlas coordinates (atlas is 16×16 tiles)
  static constexpr float kTile = 1.0f / 16.0f;
};
