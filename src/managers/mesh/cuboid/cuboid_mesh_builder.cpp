#include "managers/mesh/cuboid/cuboid_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "math3d.h"

void CuboidMeshBuilder_GenerateMesh(Block* t_block,
                                    std::vector<Vec4>* t_vertices,
                                    std::vector<Color>* t_vertices_colors,
                                    std::vector<Vec4>* t_uv_map,
                                    WorldLightModel* t_worldLightModel,
                                    LevelMap* t_terrain) {
  CuboidMeshBuilder_loadMeshData(t_block, t_vertices);
  CuboidMeshBuilder_loadUVData(t_block, t_uv_map);
  CuboidMeshBuilder_loadLightData(t_block, t_vertices_colors, t_worldLightModel,
                                  t_terrain);
}

void CuboidMeshBuilder_loadMeshData(Block* t_block,
                                    std::vector<Vec4>* t_vertices) {
  int vert;
  const Vec4* rawData = VertexBlockData::cuboidVertexData;

  if (t_block->isTopFaceVisible()) {
    vert = 0;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isBottomFaceVisible()) {
    vert = 6;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isLeftFaceVisible()) {
    vert = 12;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isRightFaceVisible()) {
    vert = 18;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isBackFaceVisible()) {
    vert = 24;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
  }
  if (t_block->isFrontFaceVisible()) {
    vert = 30;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
  }
}

void CuboidMeshBuilder_loadUVData(Block* t_block, std::vector<Vec4>* t_uv_map) {
  if (t_block->isTopFaceVisible()) {
    CuboidMeshBuilder_loadUVFaceData(t_block->facesMapIndex[0], t_uv_map);
  }
  if (t_block->isBottomFaceVisible()) {
    CuboidMeshBuilder_loadUVFaceData(t_block->facesMapIndex[1], t_uv_map);
  }
  if (t_block->isLeftFaceVisible()) {
    CuboidMeshBuilder_loadUVFaceData(t_block->facesMapIndex[2], t_uv_map);
  }
  if (t_block->isRightFaceVisible()) {
    CuboidMeshBuilder_loadUVFaceData(t_block->facesMapIndex[3], t_uv_map);
  }
  if (t_block->isBackFaceVisible()) {
    CuboidMeshBuilder_loadUVFaceData(t_block->facesMapIndex[4], t_uv_map);
  }
  if (t_block->isFrontFaceVisible()) {
    CuboidMeshBuilder_loadUVFaceData(t_block->facesMapIndex[5], t_uv_map);
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

std::array<FACE_SIDE, 4> CuboidMeshBuilder_getFaceByRotation(
    Block* t_block, LevelMap* t_terrain) {
  Vec4 tempBlockOffset;
  std::array<FACE_SIDE, 4> result = {};

  GetXYZFromPos(&t_block->offset, &tempBlockOffset);

  const BlockOrientation orientation = GetBlockOrientationDataFromMap(
      t_terrain, tempBlockOffset.x, tempBlockOffset.y, tempBlockOffset.z);

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

void CuboidMeshBuilder_loadLightData(Block* t_block,
                                     std::vector<Color>* t_vertices_colors,
                                     WorldLightModel* t_worldLightModel,
                                     LevelMap* t_terrain) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  const std::array<FACE_SIDE, 4> faceByRotation =
      CuboidMeshBuilder_getFaceByRotation(t_block, t_terrain);

  if (t_block->isTopFaceVisible()) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP,
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        CuboidMeshBuilder_getFaceNeightbors(FACE_SIDE::TOP, t_block, t_terrain);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (t_block->isBottomFaceVisible()) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BOTTOM,
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors = CuboidMeshBuilder_getFaceNeightbors(
        FACE_SIDE::BOTTOM, t_block, t_terrain);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (t_block->isLeftFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, faceByRotation[0],
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors = CuboidMeshBuilder_getFaceNeightbors(
        faceByRotation[0], t_block, t_terrain);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (t_block->isRightFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, faceByRotation[3],
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors = CuboidMeshBuilder_getFaceNeightbors(
        faceByRotation[3], t_block, t_terrain);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (t_block->isBackFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, faceByRotation[2],
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors = CuboidMeshBuilder_getFaceNeightbors(
        faceByRotation[2], t_block, t_terrain);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  if (t_block->isFrontFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, faceByRotation[1],
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors = CuboidMeshBuilder_getFaceNeightbors(
        faceByRotation[1], t_block, t_terrain);
    CuboidMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                              t_vertices_colors);
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

/**
 * Result order:
 * 7  0  1
 * 6     2
 * 5  4  3
 *
 */
std::array<u8, 8> CuboidMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                      Block* block,
                                                      LevelMap* t_terrain) {
  Vec4 pos;
  GetXYZFromPos(&block->offset, &pos);

  auto result = std::array<u8, 8>();

  switch (faceSide) {
    case FACE_SIDE::TOP:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z + 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z - 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::BOTTOM:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z - 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z + 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      break;

    case FACE_SIDE::LEFT:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z - 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z + 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::RIGHT:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z + 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z - 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      break;

    case FACE_SIDE::BACK:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z + 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z + 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z + 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z + 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::FRONT:
      result[0] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z - 1));
      result[1] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      result[2] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z - 1));
      result[3] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[4] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z - 1));
      result[5] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      result[6] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z - 1));
      result[7] = CuboidMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z - 1));
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