#include "managers/mesh/slab/slab_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void SlabMeshBuilder_GenerateMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                                  std::vector<Color>* t_vertices_colors,
                                  std::vector<Vec4>* t_uv_map,
                                  WorldLightModel* t_worldLightModel,
                                  LevelMap* t_terrain) {
  SlabMeshBuilder_loadMeshData(t_block, t_vertices, t_terrain);
  SlabMeshBuilder_loadUVData(t_block, t_uv_map);
  SlabMeshBuilder_loadLightData(t_block, t_vertices_colors, t_worldLightModel,
                                t_terrain);
}

void SlabMeshBuilder_loadMeshData(Block* t_block, std::vector<Vec4>* t_vertices,
                                  LevelMap* t_terrain) {
  int vert;
  Vec4 pos;
  GetXYZFromPos(&t_block->offset, &pos);
  const SlabOrientation orientation =
      GetSlabOrientationDataFromMap(t_terrain, pos.x, pos.y, pos.z);

  Vec4* rawData;
  if (orientation == SlabOrientation::Top) {
    rawData = (Vec4*)VertexBlockData::topSlabVertexData;
  } else {
    rawData = (Vec4*)VertexBlockData::bottomSlabVertexData;
  }

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

void SlabMeshBuilder_loadUVData(Block* t_block, std::vector<Vec4>* t_uv_map) {
  if (t_block->isTopFaceVisible()) {
    SlabMeshBuilder_loadTopDownUVFaceData(t_block->facesMapIndex[0], t_uv_map);
  }
  if (t_block->isBottomFaceVisible()) {
    SlabMeshBuilder_loadTopDownUVFaceData(t_block->facesMapIndex[1], t_uv_map);
  }
  if (t_block->isLeftFaceVisible()) {
    SlabMeshBuilder_loadSideUVFaceData(t_block->facesMapIndex[2], t_uv_map);
  }
  if (t_block->isRightFaceVisible()) {
    SlabMeshBuilder_loadSideUVFaceData(t_block->facesMapIndex[3], t_uv_map);
  }
  if (t_block->isBackFaceVisible()) {
    SlabMeshBuilder_loadSideUVFaceData(t_block->facesMapIndex[4], t_uv_map);
  }
  if (t_block->isFrontFaceVisible()) {
    SlabMeshBuilder_loadSideUVFaceData(t_block->facesMapIndex[5], t_uv_map);
  }
}

void SlabMeshBuilder_loadSideUVFaceData(const u8& index,
                                        std::vector<Vec4>* t_uv_map) {
  const u8 X = index < MAX_TEX_COLS ? index : index % MAX_TEX_COLS;
  const u8 Y = index < MAX_TEX_COLS ? 0 : std::floor(index / MAX_TEX_COLS);
  const float scale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(scale, scale, 1.0F, 0.0F);

  t_uv_map->emplace_back(Vec4(X, (Y + 0.5F), 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4((X + 1.0F), (Y + 0.5F), 1.0F, 0.0F) * scaleVec);

  t_uv_map->emplace_back(Vec4(X, (Y + 0.5F), 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
  t_uv_map->emplace_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
}

void SlabMeshBuilder_loadTopDownUVFaceData(const u8& index,
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

std::array<FACE_SIDE, 4> SlabMeshBuilder_getFaceByRotation(
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

void SlabMeshBuilder_loadLightData(Block* t_block,
                                   std::vector<Color>* t_vertices_colors,
                                   WorldLightModel* t_worldLightModel,
                                   LevelMap* t_terrain) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  const std::array<FACE_SIDE, 4> faceByRotation =
      SlabMeshBuilder_getFaceByRotation(t_block, t_terrain);

  if (t_block->isTopFaceVisible()) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    SlabMeshBuilder_ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP,
                                     t_terrain,
                                     t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        SlabMeshBuilder_getFaceNeightbors(FACE_SIDE::TOP, t_block, t_terrain);
    SlabMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                            t_vertices_colors);
  }

  if (t_block->isBottomFaceVisible()) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    SlabMeshBuilder_ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BOTTOM,
                                     t_terrain,
                                     t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors = SlabMeshBuilder_getFaceNeightbors(FACE_SIDE::BOTTOM,
                                                            t_block, t_terrain);
    SlabMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                            t_vertices_colors);
  }

  if (t_block->isLeftFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    SlabMeshBuilder_ApplyLightToFace(&faceColor, t_block, faceByRotation[0],
                                     t_terrain,
                                     t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        SlabMeshBuilder_getFaceNeightbors(faceByRotation[0], t_block, t_terrain);
    SlabMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                            t_vertices_colors);
  }

  if (t_block->isRightFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    SlabMeshBuilder_ApplyLightToFace(&faceColor, t_block, faceByRotation[3],
                                     t_terrain,
                                     t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        SlabMeshBuilder_getFaceNeightbors(faceByRotation[3], t_block, t_terrain);
    SlabMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                            t_vertices_colors);
  }

  if (t_block->isBackFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    SlabMeshBuilder_ApplyLightToFace(&faceColor, t_block, faceByRotation[2],
                                     t_terrain,
                                     t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        SlabMeshBuilder_getFaceNeightbors(faceByRotation[2], t_block, t_terrain);
    SlabMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
                                            t_vertices_colors);
  }

  if (t_block->isFrontFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    SlabMeshBuilder_ApplyLightToFace(&faceColor, t_block, faceByRotation[1],
                                     t_terrain,
                                     t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    auto faceNeightbors =
        SlabMeshBuilder_getFaceNeightbors(faceByRotation[1], t_block, t_terrain);
    SlabMeshBuilder_loadLightFaceDataWithAO(&faceColor, faceNeightbors,
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
std::array<u8, 8> SlabMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                    Block* block,
                                                    LevelMap* t_terrain) {
  Vec4 pos;
  GetXYZFromPos(&block->offset, &pos);

  auto result = std::array<u8, 8>();

  switch (faceSide) {
    case FACE_SIDE::TOP:
      result[0] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z + 1));
      result[1] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      result[2] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z));
      result[3] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      result[4] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z - 1));
      result[5] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      result[6] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z));
      result[7] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::BOTTOM:
      result[0] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z - 1));
      result[1] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[2] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z));
      result[3] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[4] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z + 1));
      result[5] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[6] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z));
      result[7] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      break;

    case FACE_SIDE::LEFT:
      result[0] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z));
      result[1] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      result[2] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z - 1));
      result[3] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      result[4] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z));
      result[5] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[6] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z + 1));
      result[7] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::RIGHT:
      result[0] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z));
      result[1] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      result[2] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z + 1));
      result[3] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[4] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z));
      result[5] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[6] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z - 1));
      result[7] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      break;

    case FACE_SIDE::BACK:
      result[0] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z + 1));
      result[1] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z + 1));
      result[2] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z + 1));
      result[3] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z + 1));
      result[4] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z + 1));
      result[5] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z + 1));
      result[6] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z + 1));
      result[7] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z + 1));
      break;

    case FACE_SIDE::FRONT:
      result[0] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y + 1, pos.z - 1));
      result[1] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y + 1, pos.z - 1));
      result[2] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y, pos.z - 1));
      result[3] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x - 1, pos.y - 1, pos.z - 1));
      result[4] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x, pos.y - 1, pos.z - 1));
      result[5] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y - 1, pos.z - 1));
      result[6] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y, pos.z - 1));
      result[7] = SlabMeshBuilder_isBlockOpaque(
          GetBlockFromMap(t_terrain, pos.x + 1, pos.y + 1, pos.z - 1));
      break;

    default:
      break;
  }

  return result;
}

// TODO: refactore to global method
bool SlabMeshBuilder_isBlockOpaque(u8 block_type) {
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

void SlabMeshBuilder_loadLightFaceDataWithAO(
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

void SlabMeshBuilder_loadLightFaceData(Color* faceColor,
                                       std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}

void SlabMeshBuilder_ApplyLightToFace(Color* baseColor, Block* targetBlock,
                                      FACE_SIDE faceSide, LevelMap* t_terrain,
                                      const float sunlightIntensity) {
  const float MAX_LIGHT_VALUE = 15.0F;
  const float MIN_LIGHT_FACTOR = 0.15F;

  u8 lightData;
  u8 sunLightLevel;
  u8 lightLevel;

  Vec4 targetBlockOffset;
  GetXYZFromPos(&targetBlock->offset, &targetBlockOffset);
  const SlabOrientation orientation = GetSlabOrientationDataFromMap(
      t_terrain, targetBlockOffset.x, targetBlockOffset.y, targetBlockOffset.z);

  switch (faceSide) {
    case FACE_SIDE::TOP:
      if (orientation == SlabOrientation::Top) {
        lightData =
            GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                                targetBlockOffset.y + 1, targetBlockOffset.z);
      } else {
        lightData =
            GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                                targetBlockOffset.y, targetBlockOffset.z);
      }

      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::BOTTOM:
      if (orientation == SlabOrientation::Top) {
        lightData =
            GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                                targetBlockOffset.y, targetBlockOffset.z);
      } else {
        lightData =
            GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                                targetBlockOffset.y - 1, targetBlockOffset.z);
      }

      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::LEFT:
      lightData = GetLightDataFromMap(t_terrain, targetBlockOffset.x + 1,
                                      targetBlockOffset.y, targetBlockOffset.z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::RIGHT:
      lightData = GetLightDataFromMap(t_terrain, targetBlockOffset.x - 1,
                                      targetBlockOffset.y, targetBlockOffset.z);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::BACK:
      lightData =
          GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                              targetBlockOffset.y, targetBlockOffset.z + 1);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    case FACE_SIDE::FRONT:
      lightData =
          GetLightDataFromMap(t_terrain, targetBlockOffset.x,
                              targetBlockOffset.y, targetBlockOffset.z - 1);
      sunLightLevel = ((lightData >> 4) & 0xF);
      lightLevel = lightData & 0x0F;
      break;

    default:
      return;
  }

  /**
   *  I've built this formula:
   * (intensity + (lightLevel / MAX_LIGHT_VALUE)) / intensity + 1.0;
   */
  const float sunLightFactor = std::max(
      (sunLightLevel * sunlightIntensity) / MAX_LIGHT_VALUE, MIN_LIGHT_FACTOR);
  const float lightLevelFactor = lightLevel / MAX_LIGHT_VALUE;

  *baseColor = LightManager::IntensifyColor(
      baseColor, std::max(sunLightFactor, lightLevelFactor));
}