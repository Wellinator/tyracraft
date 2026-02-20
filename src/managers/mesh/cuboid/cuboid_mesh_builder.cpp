#include "managers/mesh/cuboid/cuboid_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "math3d.h"
#include "managers/model_builder.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "managers/settings_manager.hpp"
#include "utils.hpp"

// ---------------------------------------------------------------------------
// Pre-computed UV lookup table (256 texture indices × 6 vertices per face)
// Eliminates per-face float division, modulo, and Vec4 construction.
// ---------------------------------------------------------------------------
struct UVFaceCache {
  Vec4 uvs[6];
};

static UVFaceCache s_uvCache[256];
static bool s_uvCacheInitialized = false;

void CuboidMeshBuilder_InitUVCache() {
  constexpr float scale = 1.0F / 16.0F;
  for (int index = 0; index < 256; ++index) {
    const int X = index % MAX_TEX_COLS;
    const int Y = index / MAX_TEX_COLS;
    const float x0 = X * scale;
    const float x1 = (X + 1) * scale;
    const float y0 = Y * scale;
    const float y1 = (Y + 1) * scale;

    // Same vertex order as the original CuboidMeshBuilder_loadUVFaceData
    s_uvCache[index].uvs[0] = Vec4(x0, y1, 1.0F, 0.0F);
    s_uvCache[index].uvs[1] = Vec4(x1, y0, 1.0F, 0.0F);
    s_uvCache[index].uvs[2] = Vec4(x1, y1, 1.0F, 0.0F);
    s_uvCache[index].uvs[3] = Vec4(x0, y1, 1.0F, 0.0F);
    s_uvCache[index].uvs[4] = Vec4(x0, y0, 1.0F, 0.0F);
    s_uvCache[index].uvs[5] = Vec4(x1, y0, 1.0F, 0.0F);
  }
  s_uvCacheInitialized = true;
}

void CuboidMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                    const int lod,
                                    std::vector<Vec4>* t_vertices,
                                    std::vector<Color>* t_vertices_colors,
                                    std::vector<Vec4>* t_uv_map,
                                    WorldLightModel* t_worldLightModel,
                                    Level* pLevel, CustomMeshOptions* options) {
  CuboidMeshBuilder_loadMeshData(offset, visibleFaces, lod, t_vertices,
                                 options);
  CuboidMeshBuilder_loadUVData(offset, visibleFaces, t_uv_map);
  CuboidMeshBuilder_loadLightData(offset, visibleFaces, t_vertices_colors,
                                  t_worldLightModel, pLevel);
}

void CuboidMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                    const int lod,
                                    std::vector<Vec4>* t_vertices,
                                    CustomMeshOptions* options) {
  int vert;
  const Vec4* rawData = VertexBlockData::cuboidVertexData;

  M4x4 model = options ? ModelBuilder_DefaultModel(
                             const_cast<Vec4*>(offset), options->scale,
                             options->rotation, options->translation)
                       : ModelBuilder_DefaultModel(const_cast<Vec4*>(offset));

  if (visibleFaces & (int)BlockFace::TOP) {
    vert = 0;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
  if (visibleFaces & (int)BlockFace::BOTTOM) {
    vert = 6;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
  if (visibleFaces & (int)BlockFace::LEFT) {
    vert = 12;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
  if (visibleFaces & (int)BlockFace::RIGHT) {
    vert = 18;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
  if (visibleFaces & (int)BlockFace::BACK) {
    vert = 24;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
  if (visibleFaces & (int)BlockFace::FRONT) {
    vert = 30;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
}

void CuboidMeshBuilder_loadUVData(const Vec4* offset, const u8 visibleFaces,
                                  std::vector<Vec4>* t_uv_map) {
  Level* pLevel = Level::getInstance();
  const Blocks block_type = static_cast<Blocks>(
      pLevel->GetBlockFromMap(offset->x, offset->y, offset->z));
  Block* blockTemplate =
      StaticBlockRepository::getInstance()->getBlockTemplate(block_type);
  u8* facesMap = blockTemplate->getFacesMap().data();

  if (visibleFaces & (int)BlockFace::TOP) {
    CuboidMeshBuilder_loadUVFaceData(facesMap[0], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::BOTTOM) {
    CuboidMeshBuilder_loadUVFaceData(facesMap[1], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::LEFT) {
    CuboidMeshBuilder_loadUVFaceData(facesMap[2], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::RIGHT) {
    CuboidMeshBuilder_loadUVFaceData(facesMap[3], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::BACK) {
    CuboidMeshBuilder_loadUVFaceData(facesMap[4], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::FRONT) {
    CuboidMeshBuilder_loadUVFaceData(facesMap[5], t_uv_map);
  }
}

void CuboidMeshBuilder_loadUVFaceData(const u8& index,
                                      std::vector<Vec4>* t_uv_map) {
  if (!s_uvCacheInitialized) CuboidMeshBuilder_InitUVCache();
  const UVFaceCache& cached = s_uvCache[index];
  t_uv_map->emplace_back(cached.uvs[0]);
  t_uv_map->emplace_back(cached.uvs[1]);
  t_uv_map->emplace_back(cached.uvs[2]);
  t_uv_map->emplace_back(cached.uvs[3]);
  t_uv_map->emplace_back(cached.uvs[4]);
  t_uv_map->emplace_back(cached.uvs[5]);
}

std::array<FACE_SIDE, 4> CuboidMeshBuilder_getFaceByRotation(const Vec4* offset,
                                                             Level* pLevel) {
  std::array<FACE_SIDE, 4> result = {};
  const BlockOrientation orientation =
      pLevel->GetBlockOrientationDataFromMap(offset->x, offset->y, offset->z);

  switch (orientation) {
    case BlockOrientation::North:
      // Will be rotated by 90deg
      // Left turns Back & Right turns Front
      // [ left, front, back, right]
      result[0] = FACE_SIDE::FRONT;
      result[1] = FACE_SIDE::RIGHT;
      result[2] = FACE_SIDE::LEFT;
      result[3] = FACE_SIDE::BACK;
      break;

    case BlockOrientation::West:
      // Will be rotated by 180deg
      // Left turns Right & Front turns Back
      // [ left, front, back, right]
      result[0] = FACE_SIDE::RIGHT;
      result[1] = FACE_SIDE::BACK;
      result[2] = FACE_SIDE::FRONT;
      result[3] = FACE_SIDE::LEFT;
      break;

    case BlockOrientation::South:
      // Will be rotated by 270deg
      // Left turns Front & Right turns Back
      // [ left, front, back, right]
      result[0] = FACE_SIDE::BACK;
      result[1] = FACE_SIDE::LEFT;
      result[2] = FACE_SIDE::RIGHT;
      result[3] = FACE_SIDE::FRONT;
      break;

    case BlockOrientation::East:
    default:
      result[0] = FACE_SIDE::LEFT;
      result[1] = FACE_SIDE::FRONT;
      result[2] = FACE_SIDE::BACK;
      result[3] = FACE_SIDE::RIGHT;
      break;
  }

  return result;
}

void CuboidMeshBuilder_loadLightData(const Vec4* offset, const u8 visibleFaces,
                                     std::vector<Color>* t_vertices_colors,
                                     WorldLightModel* t_worldLightModel,
                                     Level* pLevel) {
  const auto baseFaceColor = Color(120, 120, 120, 128);
  const std::array<FACE_SIDE, 4> faceByRotation =
      CuboidMeshBuilder_getFaceByRotation(offset, pLevel);

  // Map face index → world FACE_SIDE (rotation-corrected for side faces)
  // Index: 0=TOP, 1=BOTTOM, 2=LEFT, 3=RIGHT, 4=BACK, 5=FRONT
  const FACE_SIDE faceSides[6] = {
      FACE_SIDE::TOP,     // TOP - always direct
      FACE_SIDE::BOTTOM,  // BOTTOM - always direct
      faceByRotation[0],  // LEFT → rotated
      faceByRotation[3],  // RIGHT → rotated
      faceByRotation[2],  // BACK → rotated
      faceByRotation[1]   // FRONT → rotated
  };

  // Face shading intensities (same as original per-face values)
  static const float faceIntensities[6] = {
      1.0F,  // TOP = 100%
      0.5F,  // BOTTOM = 50%
      0.6F,  // LEFT = 60%
      0.6F,  // RIGHT = 60%
      0.8F,  // BACK = 80%
      0.8F   // FRONT = 80%
  };

  // Batch compute all face light colors in one call
  Color faceColors[6];
  LightManager::ApplyLightToAllFaces(baseFaceColor, offset, visibleFaces,
                                     faceSides, faceIntensities, pLevel,
                                     t_worldLightModel->sunLightIntensity,
                                     faceColors);

  // BlockFace bit flags matching the face index order
  static const u8 faceBits[6] = {
      (u8)BlockFace::TOP, (u8)BlockFace::BOTTOM, (u8)BlockFace::LEFT,
      (u8)BlockFace::RIGHT, (u8)BlockFace::BACK, (u8)BlockFace::FRONT};

  // Emit vertex colors for each visible face (with AO if enabled)
  for (int f = 0; f < 6; ++f) {
    if (!(visibleFaces & faceBits[f])) continue;

    Color faceColor = faceColors[f];

    if (g_settings.ambient_occlusion) {
      auto faceNeighbors =
          CuboidMeshBuilder_getFaceNeighbors(faceSides[f], offset, pLevel);
      CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeighbors,
                                                t_vertices_colors);
    } else {
      CuboidMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
    }
  }
}

/**
 * Result order:
 * 7  0  1
 * 6     2
 * 5  4  3
 *
 */
std::array<u8, 8> CuboidMeshBuilder_getFaceNeighbors(FACE_SIDE faceSide,
                                                     const Vec4* offset,
                                                     Level* pLevel) {
  auto result = std::array<u8, 8>();

  switch (faceSide) {
    case FACE_SIDE::TOP:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y + 1, offset->z + 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z + 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z - 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y + 1, offset->z - 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z - 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z + 1));
      break;

    case FACE_SIDE::BOTTOM:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y - 1, offset->z - 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z - 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z + 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y - 1, offset->z + 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z + 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z - 1));
      break;

    case FACE_SIDE::LEFT:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z - 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y, offset->z - 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z - 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z + 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y, offset->z + 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z + 1));
      break;

    case FACE_SIDE::RIGHT:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z + 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y, offset->z + 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z + 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z - 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y, offset->z - 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z - 1));
      break;

    case FACE_SIDE::BACK:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y + 1, offset->z + 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z + 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y, offset->z + 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z + 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y - 1, offset->z + 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z + 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y, offset->z + 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z + 1));
      break;

    case FACE_SIDE::FRONT:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y + 1, offset->z - 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y + 1, offset->z - 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y, offset->z - 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x - 1, offset->y - 1, offset->z - 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x, offset->y - 1, offset->z - 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y - 1, offset->z - 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y, offset->z - 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          pLevel->GetBlockFromMap(offset->x + 1, offset->y + 1, offset->z - 1));
      break;

    default:
      break;
  }

  return result;
}

// TODO: refactore to global method
bool CuboidMeshBuilder_isBlockOpaque(u8 block_type) {
  return block_type != (u8)Blocks::VOID &&
         block_type != (u8)Blocks::AIR_BLOCK &&
         block_type != (u8)Blocks::GLASS_BLOCK &&
         block_type != (u8)Blocks::POPPY_FLOWER &&
         block_type != (u8)Blocks::DANDELION_FLOWER &&
         block_type != (u8)Blocks::GRASS &&
         block_type != (u8)Blocks::WATER_BLOCK &&
         block_type != (u8)Blocks::LAVA_BLOCK &&
         block_type != (u8)Blocks::TORCH;
}

void CuboidMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors) {
  std::array<u8, 4> AOCornersValues =
      LightManager::getCornersAOValues(faceNeightbors);

  // DEBUG Vertices Colors
  // t_vertices_colors->emplace_back(Color(255, 0, 0));
  // t_vertices_colors->emplace_back(Color(0, 255, 0));
  // t_vertices_colors->emplace_back(Color(0, 0, 255));
  // t_vertices_colors->emplace_back(Color(255, 255, 0));
  // t_vertices_colors->emplace_back(Color(0, 255, 255));
  // t_vertices_colors->emplace_back(Color(255, 0, 255));

  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      *faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      *faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      *faceColor, LightManager::calcAOIntensity(AOCornersValues[1])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      *faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      *faceColor, LightManager::calcAOIntensity(AOCornersValues[2])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      *faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
}

void CuboidMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}