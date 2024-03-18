#include "managers/mesh/crossed/crossed_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void CrossedMeshBuilder_GenerateMesh(Block* t_block,
                                     std::vector<Vec4>* t_vertices,
                                     std::vector<Color>* t_vertices_colors,
                                     std::vector<Vec4>* t_uv_map,
                                     WorldLightModel* t_worldLightModel,
                                     LevelMap* t_terrain) {
  CrossedMeshBuilder_loadCrossedMeshData(t_block, t_vertices);
  CrossedMeshBuilder_loadCrossedUVData(t_block, t_uv_map);
  CrossedMeshBuilder_loadCroosedLightData(t_block, t_vertices_colors,
                                          t_worldLightModel, t_terrain);
}

void CrossedMeshBuilder_loadCrossedMeshData(Block* t_block,
                                            std::vector<Vec4>* t_vertices) {
  int vert = 0;
  const Vec4* crossBlockRawData = VertexBlockData::crossedVertexData;

  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);

  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE +
                           t_block->position);
}

void CrossedMeshBuilder_loadCrossedUVData(Block* t_block,
                                          std::vector<Vec4>* t_uv_map) {
  CrossedMeshBuilder_loadUVFaceData(t_block->facesMapIndex[0], t_uv_map);
  CrossedMeshBuilder_loadUVFaceData(t_block->facesMapIndex[0], t_uv_map);
}

void CrossedMeshBuilder_loadUVFaceData(const u8& index,
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

void CrossedMeshBuilder_loadCroosedLightData(
    Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel, LevelMap* t_terrain) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  // Face 1
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP,
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    CrossedMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  // Face 2
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP,
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    CrossedMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void CrossedMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}