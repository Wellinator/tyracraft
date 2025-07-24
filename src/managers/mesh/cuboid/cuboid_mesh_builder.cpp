#include "managers/mesh/cuboid/cuboid_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "math3d.h"
#include "managers/model_builder.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "utils.hpp"

void CuboidMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                    std::vector<Vec4>* t_vertices,
                                    std::vector<Color>* t_vertices_colors,
                                    std::vector<Vec4>* t_uv_map,
                                    WorldLightModel* t_worldLightModel,
                                    Level* pLevel) {
  CuboidMeshBuilder_loadMeshData(offset, visibleFaces, t_vertices);
  CuboidMeshBuilder_loadUVData(offset, visibleFaces, t_uv_map);
  CuboidMeshBuilder_loadLightData(offset, visibleFaces, t_vertices_colors,
                                  t_worldLightModel, pLevel);
}

void CuboidMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                    std::vector<Vec4>* t_vertices) {
  int vert;
  const Vec4* rawData = VertexBlockData::cuboidVertexData;
  M4x4 model = ModelBuilder_DefaultModel(const_cast<Vec4*>(offset));

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
  const u8 X = index < MAX_TEX_COLS ? index : index % MAX_TEX_COLS;
  const u8 Y = index < MAX_TEX_COLS ? 0 : std::floor(index / MAX_TEX_COLS);
  const float scale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(scale, scale, 1.0F, 0.0F);

  t_uv_map->emplace_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

  t_uv_map->emplace_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
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
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;
  const std::array<FACE_SIDE, 4> faceByRotation =
      CuboidMeshBuilder_getFaceByRotation(offset, pLevel);

  if (visibleFaces & (int)BlockFace::TOP) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(FACE_SIDE::TOP, offset, pLevel);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::BOTTOM) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::BOTTOM, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(FACE_SIDE::BOTTOM, offset, pLevel);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::LEFT) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   faceByRotation[0], pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(faceByRotation[0], offset, pLevel);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::RIGHT) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   faceByRotation[3], pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(faceByRotation[3], offset, pLevel);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::BACK) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   faceByRotation[2], pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(faceByRotation[2], offset, pLevel);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::FRONT) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   faceByRotation[1], pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(faceByRotation[1], offset, pLevel);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  const u8 visibleFacesCount = Utils::countSetBits(visibleFaces);
  blockColorAverage /= visibleFacesCount;
  //   t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
  //                          blockColorAverage.z);
}

/**
 * Result order:
 * 7  0  1
 * 6     2
 * 5  4  3
 *
 */
std::array<u8, 8> CuboidMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
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
      faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[1])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[0])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[2])));
  t_vertices_colors->emplace_back(LightManager::IntensifyColor(
      faceColor, LightManager::calcAOIntensity(AOCornersValues[3])));
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