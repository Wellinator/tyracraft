#include "managers/mesh/cuboid/cuboid_hand_item_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void CuboidHandItemMeshBuilder_GenerateMesh(
    Block* t_block, std::vector<Vec4>* t_vertices,
    std::vector<Color>* t_vertices_colors, std::vector<Vec4>* t_uv_map,
    WorldLightModel* t_worldLightModel) {
  CuboidHandItemMeshBuilder_loadMeshData(t_block, t_vertices);
  CuboidHandItemMeshBuilder_loadUVData(t_block, t_uv_map);
  if (t_vertices_colors) {
    CuboidHandItemMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                            t_worldLightModel);
  }
}

void CuboidHandItemMeshBuilder_loadMeshData(Block* t_block,
                                            std::vector<Vec4>* t_vertices) {
  int vert;
  const Vec4* rawData = VertexBlockData::getVertexData();

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

  delete rawData;
}

void CuboidHandItemMeshBuilder_loadUVData(Block* t_block,
                                          std::vector<Vec4>* t_uv_map) {
  if (t_block->isTopFaceVisible()) {
    CuboidHandItemMeshBuilder_loadUVFaceData(t_block->facesMapIndex[0],
                                             t_uv_map);
  }
  if (t_block->isBottomFaceVisible()) {
    CuboidHandItemMeshBuilder_loadUVFaceData(t_block->facesMapIndex[1],
                                             t_uv_map);
  }
  if (t_block->isLeftFaceVisible()) {
    CuboidHandItemMeshBuilder_loadUVFaceData(t_block->facesMapIndex[2],
                                             t_uv_map);
  }
  if (t_block->isRightFaceVisible()) {
    CuboidHandItemMeshBuilder_loadUVFaceData(t_block->facesMapIndex[3],
                                             t_uv_map);
  }
  if (t_block->isBackFaceVisible()) {
    CuboidHandItemMeshBuilder_loadUVFaceData(t_block->facesMapIndex[4],
                                             t_uv_map);
  }
  if (t_block->isFrontFaceVisible()) {
    CuboidHandItemMeshBuilder_loadUVFaceData(t_block->facesMapIndex[5],
                                             t_uv_map);
  }
}

void CuboidHandItemMeshBuilder_loadUVFaceData(const u8& index,
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

void CuboidHandItemMeshBuilder_loadLightData(
    Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  // const float MAX_LIGHT_VALUE = 15.0F;
  // const float MIN_LIGHT_FACTOR = 0.25F;

  {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // TODO: calc light by user position
    // const float sunLightFactor =
    //     std::max(t_worldLightModel->sunLightIntensity / MAX_LIGHT_VALUE,
    //              MIN_LIGHT_FACTOR);
    // faceColor = LightManager::IntensifyColor(&faceColor, sunLightFactor);

    // Vec4::copy(&tempColor, faceColor.rgba);
    // blockColorAverage += tempColor;

    CuboidHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // TODO: calc light by user position
    // const float sunLightFactor =
    //     std::max(t_worldLightModel->sunLightIntensity / MAX_LIGHT_VALUE,
    //              MIN_LIGHT_FACTOR);
    // faceColor = LightManager::IntensifyColor(&faceColor, sunLightFactor);

    // Vec4::copy(&tempColor, faceColor.rgba);
    // blockColorAverage += tempColor;

    CuboidHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // TODO: calc light by user position
    // const float sunLightFactor =
    //     std::max(t_worldLightModel->sunLightIntensity / MAX_LIGHT_VALUE,
    //              MIN_LIGHT_FACTOR);
    // faceColor = LightManager::IntensifyColor(&faceColor, sunLightFactor);

    // Vec4::copy(&tempColor, faceColor.rgba);
    // blockColorAverage += tempColor;

    CuboidHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // TODO: calc light by user position
    // const float sunLightFactor =
    //     std::max(t_worldLightModel->sunLightIntensity / MAX_LIGHT_VALUE,
    //              MIN_LIGHT_FACTOR);
    // faceColor = LightManager::IntensifyColor(&faceColor, sunLightFactor);

    // Vec4::copy(&tempColor, faceColor.rgba);
    // blockColorAverage += tempColor;

    CuboidHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // TODO: calc light by user position
    // const float sunLightFactor =
    //     std::max(t_worldLightModel->sunLightIntensity / MAX_LIGHT_VALUE,
    //              MIN_LIGHT_FACTOR);
    // faceColor = LightManager::IntensifyColor(&faceColor, sunLightFactor);

    // Vec4::copy(&tempColor, faceColor.rgba);
    // blockColorAverage += tempColor;

    CuboidHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // TODO: calc light by user position
    // const float sunLightFactor =
    //     std::max(t_worldLightModel->sunLightIntensity / MAX_LIGHT_VALUE,
    //              MIN_LIGHT_FACTOR);
    // faceColor = LightManager::IntensifyColor(&faceColor, sunLightFactor);

    // Vec4::copy(&tempColor, faceColor.rgba);
    // blockColorAverage += tempColor;

    CuboidHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void CuboidHandItemMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}