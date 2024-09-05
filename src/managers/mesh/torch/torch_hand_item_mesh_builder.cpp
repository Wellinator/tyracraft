#include "managers/mesh/torch/torch_hand_item_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void TorchHandItemMeshBuilder_GenerateMesh(
    Block* t_block, std::vector<Vec4>* t_vertices,
    std::vector<Color>* t_vertices_colors, std::vector<Vec4>* t_uv_map,
    WorldLightModel* t_worldLightModel) {
  TorchHandItemMeshBuilder_loadMeshData(t_block, t_vertices);
  TorchHandItemMeshBuilder_loadUVData(t_uv_map);
  if (t_vertices_colors) {
    TorchHandItemMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                           t_worldLightModel);
  }
}

void TorchHandItemMeshBuilder_loadMeshData(Block* t_block,
                                           std::vector<Vec4>* t_vertices) {
  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_vertices->emplace_back(t_block->model *
                             VertexBlockData::torchVertexData[i]);
  }
}

void TorchHandItemMeshBuilder_loadUVData(std::vector<Vec4>* t_uv_map) {
  const Vec4* UVData = VertexBlockData::getTorchUVData();
  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_uv_map->emplace_back(UVData[i]);
  }
  delete UVData;
}

void TorchHandItemMeshBuilder_loadLightData(
    Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  for (size_t i = 0; i < 6; i++) {
    Color faceColor = baseFaceColor;
    TorchHandItemMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void TorchHandItemMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);

  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}
