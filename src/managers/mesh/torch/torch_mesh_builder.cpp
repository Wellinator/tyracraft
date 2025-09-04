#include "managers/mesh/torch/torch_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "managers/model_builder.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "utils.hpp"

void TorchMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                   std::vector<Vec4>* t_vertices,
                                   std::vector<Color>* t_vertices_colors,
                                   std::vector<Vec4>* t_uv_map,
                                   WorldLightModel* t_worldLightModel,
                                   Level* pLevel) {
  TorchMeshBuilder_loadMeshData(offset, visibleFaces, t_vertices);
  TorchMeshBuilder_loadUVData(t_uv_map);
  TorchMeshBuilder_loadLightData(offset, visibleFaces, t_vertices_colors,
                                 t_worldLightModel, pLevel);
}

void TorchMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                   std::vector<Vec4>* t_vertices) {
  M4x4 model = ModelBuilder_TorchModel(const_cast<Vec4*>(offset));

  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_vertices->emplace_back(model * VertexBlockData::torchVertexData[i]);
  }
}

void TorchMeshBuilder_loadUVData(std::vector<Vec4>* t_uv_map) {
  const Vec4* UVData = VertexBlockData::getTorchUVData();

  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_uv_map->emplace_back(UVData[i]);
  }

  delete UVData;
}

void TorchMeshBuilder_loadLightData(const Vec4* offset, const u8 visibleFaces,
                                    std::vector<Color>* t_vertices_colors,
                                    WorldLightModel* t_worldLightModel,
                                    Level* pLevel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 tempColor;

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;
    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }
}

void TorchMeshBuilder_loadLightFaceData(Color* faceColor,
                                        std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);

  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}
