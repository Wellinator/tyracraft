#include "managers/mesh/torch/torch_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void TorchMeshBuilder_GenerateMesh(Block* t_block,
                                   std::vector<Vec4>* t_vertices,
                                   std::vector<Color>* t_vertices_colors,
                                   std::vector<Vec4>* t_uv_map,
                                   WorldLightModel* t_worldLightModel,
                                   LevelMap* t_terrain) {
  Vec4 pos;
  GetXYZFromPos(&t_block->offset, &pos);

  const BlockOrientation orientation =
      GetTorchOrientationDataFromMap(t_terrain, pos.x, pos.y, pos.z);

  TorchMeshBuilder_loadMeshData(t_block, t_vertices, orientation);
  TorchMeshBuilder_loadUVData(t_uv_map);
  TorchMeshBuilder_loadLightData(t_block, t_vertices_colors, t_worldLightModel,
                                 t_terrain);
}

void TorchMeshBuilder_loadMeshData(Block* t_block,
                                   std::vector<Vec4>* t_vertices,
                                   const BlockOrientation orientation) {
  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_vertices->emplace_back(t_block->model *
                             VertexBlockData::torchVertexData[i]);
  }
}

void TorchMeshBuilder_loadUVData(std::vector<Vec4>* t_uv_map) {
  const Vec4* UVData = VertexBlockData::getTorchUVData();

  for (size_t i = 0; i < VertexBlockData::VETEX_COUNT; i++) {
    t_uv_map->emplace_back(UVData[i]);
  }

  delete UVData;
}

void TorchMeshBuilder_loadLightData(Block* t_block,
                                    std::vector<Color>* t_vertices_colors,
                                    WorldLightModel* t_worldLightModel,
                                    LevelMap* t_terrain) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, t_terrain,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;
    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  {
    Color faceColor = baseFaceColor;

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    TorchMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  blockColorAverage /= t_block->visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
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
