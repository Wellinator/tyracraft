#include "managers/mesh/crossed/crossed_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "managers/block_manager.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "managers/model_builder.hpp"
#include "utils.hpp"

void CrossedMeshBuilder_GenerateMesh(
    const Vec4* offset, const u8 visibleFaces, const int lod,
    std::vector<Vec4>* t_vertices, std::vector<Color>* t_vertices_colors,
    std::vector<Vec4>* t_uv_map, WorldLightModel* t_worldLightModel,
    Level* pLevel, CustomMeshOptions* options) {
  if (lod > 0) return;
  CrossedMeshBuilder_loadCrossedMeshData(offset, t_vertices, options);
  CrossedMeshBuilder_loadCrossedUVData(offset, t_uv_map);
  CrossedMeshBuilder_loadCrossedLightData(
      offset, visibleFaces, t_vertices_colors, t_worldLightModel, pLevel);
}

void CrossedMeshBuilder_loadCrossedMeshData(const Vec4* offset,
                                            std::vector<Vec4>* t_vertices,
                                            CustomMeshOptions* options) {
  int vert = 0;
  const Vec4* crossBlockRawData = VertexBlockData::crossedVertexData;

  float _scale = BLOCK_SIZE;
  Vec4 _position = Level::getInstance()->offsetToWorldPos(offset);

  if (options) {
    if (options->scale != 0) {
      _scale *= options->scale;
    }

    if (options->translation.length() != 0) {
      _position += options->translation;
    }
  }

  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);

  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * _scale + _position);
}

void CrossedMeshBuilder_loadCrossedUVData(const Vec4* offset,
                                          std::vector<Vec4>* t_uv_map) {
  Level* pLevel = Level::getInstance();
  const Blocks block_type = static_cast<Blocks>(
      pLevel->GetBlockFromMap(offset->x, offset->y, offset->z));
  Block* blockTemplate =
      StaticBlockRepository::getInstance()->getBlockTemplate(block_type);
  u8* facesMap = blockTemplate->getFacesMap().data();
  CrossedMeshBuilder_loadUVFaceData(facesMap[0], t_uv_map);
  CrossedMeshBuilder_loadUVFaceData(facesMap[0], t_uv_map);
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

void CrossedMeshBuilder_loadCrossedLightData(
    const Vec4* offset, const u8 visibleFaces,
    std::vector<Color>* t_vertices_colors, WorldLightModel* t_worldLightModel,
    Level* pLevel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 tempColor;

  // Face 1
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    CrossedMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  // Face 2
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    CrossedMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }
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