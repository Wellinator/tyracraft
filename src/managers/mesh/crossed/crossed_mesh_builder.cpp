#include "managers/mesh/crossed/crossed_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "managers/block_manager.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "managers/model_builder.hpp"
#include "utils.hpp"

void CrossedMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                     std::vector<Vec4>* t_vertices,
                                     std::vector<Color>* t_vertices_colors,
                                     std::vector<Vec4>* t_uv_map,
                                     WorldLightModel* t_worldLightModel,
                                     Level* pLevel) {
  CrossedMeshBuilder_loadCrossedMeshData(offset, t_vertices);
  CrossedMeshBuilder_loadCrossedUVData(offset, t_uv_map);
  CrossedMeshBuilder_loadCrossedLightData(
      offset, visibleFaces, t_vertices_colors, t_worldLightModel, pLevel);
}

void CrossedMeshBuilder_loadCrossedMeshData(const Vec4* offset,
                                            std::vector<Vec4>* t_vertices) {
  int vert = 0;
  const Vec4* crossBlockRawData = VertexBlockData::crossedVertexData;
  Vec4 position = Level::getInstance()->offsetToWorldPos(offset);

  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);

  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
  t_vertices->emplace_back(crossBlockRawData[vert++] * BLOCK_SIZE + position);
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
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  // Face 1
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    CrossedMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  // Face 2
  {
    Color faceColor = baseFaceColor;
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    CrossedMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  const u8 visibleFacesCount = Utils::countSetBits(visibleFaces);
  blockColorAverage /= visibleFacesCount;
  // t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
  //                        blockColorAverage.z);
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