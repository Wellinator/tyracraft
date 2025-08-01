#include "managers/mesh/slab/slab_hand_item_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void SlabHandItemMeshBuilder_GenerateMesh(Block* t_block,
                                          std::vector<Vec4>* t_vertices,
                                          std::vector<Color>* t_vertices_colors,
                                          std::vector<Vec4>* t_uv_map,
                                          WorldLightModel* t_worldLightModel) {
  SlabHandItemMeshBuilder_loadMeshData(t_block, t_vertices);
  SlabHandItemMeshBuilder_loadUVData(t_block, t_uv_map);
  if (t_vertices_colors) {
    SlabHandItemMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                          t_worldLightModel);
  }
}

void SlabHandItemMeshBuilder_loadMeshData(Block* t_block,
                                          std::vector<Vec4>* t_vertices) {
  const Vec4* rawData = VertexBlockData::getBottomSlabVertexData();

  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_vertices->emplace_back(t_block->model * rawData[i]);
  }

  delete rawData;
}

void SlabHandItemMeshBuilder_loadUVData(Block* t_block,
                                        std::vector<Vec4>* t_uv_map) {
  u8* facesMap = t_block->getFacesMap().data();

  if (t_block->isTopFaceVisible()) {
    SlabHandItemMeshBuilder_loadTopDownUVFaceData(facesMap[0], t_uv_map);
  }
  if (t_block->isBottomFaceVisible()) {
    SlabHandItemMeshBuilder_loadTopDownUVFaceData(facesMap[1], t_uv_map);
  }
  if (t_block->isLeftFaceVisible()) {
    SlabHandItemMeshBuilder_loadSideUVFaceData(facesMap[2], t_uv_map);
  }
  if (t_block->isRightFaceVisible()) {
    SlabHandItemMeshBuilder_loadSideUVFaceData(facesMap[3], t_uv_map);
  }
  if (t_block->isBackFaceVisible()) {
    SlabHandItemMeshBuilder_loadSideUVFaceData(facesMap[4], t_uv_map);
  }
  if (t_block->isFrontFaceVisible()) {
    SlabHandItemMeshBuilder_loadSideUVFaceData(facesMap[5], t_uv_map);
  }
}

void SlabHandItemMeshBuilder_loadSideUVFaceData(const u8& index,
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

void SlabHandItemMeshBuilder_loadTopDownUVFaceData(
    const u8& index, std::vector<Vec4>* t_uv_map) {
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

void SlabHandItemMeshBuilder_loadLightData(
    Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Color faceColor;

  faceColor = LightManager::IntensifyColor(baseFaceColor, 1.0F);
  SlabHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);

  faceColor = LightManager::IntensifyColor(baseFaceColor, 0.5F);
  SlabHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);

  faceColor = LightManager::IntensifyColor(baseFaceColor, 0.6F);
  SlabHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);

  faceColor = LightManager::IntensifyColor(baseFaceColor, 0.6F);
  SlabHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);

  faceColor = LightManager::IntensifyColor(baseFaceColor, 0.8F);
  SlabHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);

  faceColor = LightManager::IntensifyColor(baseFaceColor, 0.8F);
  SlabHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);

  blockColorAverage /= t_block->getVisibleFacesCount();
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void SlabHandItemMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}