#include "managers/mesh/water/water_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "models/liquid_quad_map_model.hpp"
#include "managers/liquid_helper.hpp"
#include "managers/model_builder.hpp"
#include "managers/block/StaticBlockRepository.hpp"
#include "utils.hpp"

void WaterMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                   const int lod, std::vector<Vec4>* t_vertices,
                                   std::vector<Color>* t_vertices_colors,
                                   std::vector<Vec4>* t_uv_map,
                                   WorldLightModel* t_worldLightModel,
                                   Level* pLevel, CustomMeshOptions* options) {
  WaterMeshBuilder_loadMeshData(offset, visibleFaces, lod, t_vertices, pLevel);
  WaterMeshBuilder_loadUVData(offset, visibleFaces, t_uv_map, pLevel);
  WaterMeshBuilder_loadLightData(offset, visibleFaces, t_vertices_colors,
                                 t_worldLightModel, pLevel);
}

void WaterMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                   const int lod, std::vector<Vec4>* t_vertices,
                                   Level* pLevel) {
  const LiquidOrientation orientation =
      pLevel->GetLiquidOrientationDataFromMap(offset->x, offset->y, offset->z);
  const LiquidQuadMapModel quadMap = LiquidHelper_getQuadMap(
      pLevel, orientation, const_cast<Vec4*>(offset), (u8)Blocks::WATER_BLOCK);

  WaterMeshBuilder_loadMeshDataByLevel(offset, visibleFaces, t_vertices,
                                       quadMap);
}

void WaterMeshBuilder_loadMeshDataByLevel(const Vec4* offset,
                                          const u8 visibleFaces,
                                          std::vector<Vec4>* t_vertices,
                                          const LiquidQuadMapModel quadMap) {
  u8 vert = 0;
  const Vec4* rawData = VertexBlockData::cuboidVertexData;

  Vec4 modelNW = Vec4(0.0F, quadMap.NW, 0.0F);
  Vec4 modelNE = Vec4(0.0F, quadMap.NE, 0.0F);
  Vec4 modelSE = Vec4(0.0F, quadMap.SE, 0.0F);
  Vec4 modelSW = Vec4(0.0F, quadMap.SW, 0.0F);

  M4x4 model = ModelBuilder_NoRotationModel(const_cast<Vec4*>(offset));

  if (visibleFaces & (int)BlockFace::TOP) {
    vert = 0;
    t_vertices->emplace_back(model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(model * rawData[vert++] - modelNE);
    t_vertices->emplace_back(model * rawData[vert++] - modelSE);

    t_vertices->emplace_back(model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(model * rawData[vert++] - modelNW);
    t_vertices->emplace_back(model * rawData[vert++] - modelNE);
  }
  if (visibleFaces & (int)BlockFace::BOTTOM) {
    vert = 6;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++]);
  }
  if (visibleFaces & (int)BlockFace::LEFT) {
    vert = 12;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(model * rawData[vert++]);

    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelNW);
    t_vertices->emplace_back(model * rawData[vert++] - modelSW);
  }
  if (visibleFaces & (int)BlockFace::RIGHT) {
    vert = 18;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelNE);
    t_vertices->emplace_back(model * rawData[vert++]);

    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelSE);
    t_vertices->emplace_back(model * rawData[vert++] - modelNE);
  }
  if (visibleFaces & (int)BlockFace::BACK) {
    vert = 24;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelNW);
    t_vertices->emplace_back(model * rawData[vert++]);

    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelNE);
    t_vertices->emplace_back(model * rawData[vert++] - modelNW);
  }
  if (visibleFaces & (int)BlockFace::FRONT) {
    vert = 30;
    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelSE);
    t_vertices->emplace_back(model * rawData[vert++]);

    t_vertices->emplace_back(model * rawData[vert++]);
    t_vertices->emplace_back(model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(model * rawData[vert++] - modelSE);
  }
}

void WaterMeshBuilder_loadUVData(const Vec4* offset, const u8 visibleFaces,
                                 std::vector<Vec4>* t_uv_map, Level* pLevel) {
  const Blocks block_type = static_cast<Blocks>(
      pLevel->GetBlockFromMap(offset->x, offset->y, offset->z));
  Block* blockTemplate =
      StaticBlockRepository::getInstance()->getBlockTemplate(block_type);
    const auto facesMap = blockTemplate->getFacesMap();

  if (visibleFaces & (int)BlockFace::TOP) {
    WaterMeshBuilder_loadUVFaceData(facesMap[0], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::BOTTOM) {
    WaterMeshBuilder_loadUVFaceData(facesMap[1], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::LEFT) {
    WaterMeshBuilder_loadUVFaceData(facesMap[2], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::RIGHT) {
    WaterMeshBuilder_loadUVFaceData(facesMap[3], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::BACK) {
    WaterMeshBuilder_loadUVFaceData(facesMap[4], t_uv_map);
  }
  if (visibleFaces & (int)BlockFace::FRONT) {
    WaterMeshBuilder_loadUVFaceData(facesMap[5], t_uv_map);
  }
}

void WaterMeshBuilder_loadUVFaceData(const u8& index,
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

void WaterMeshBuilder_loadLightData(const Vec4* offset, const u8 visibleFaces,
                                    std::vector<Color>* t_vertices_colors,
                                    WorldLightModel* t_worldLightModel,
                                    Level* pLevel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 tempColor;

  if (visibleFaces & (int)BlockFace::TOP) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    WaterMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::BOTTOM) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::BOTTOM, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    WaterMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::LEFT) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::LEFT, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    WaterMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::RIGHT) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::RIGHT, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    WaterMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::BACK) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::BACK, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    WaterMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (visibleFaces & (int)BlockFace::FRONT) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, const_cast<Vec4*>(offset),
                                   FACE_SIDE::FRONT, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    WaterMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }
}

void WaterMeshBuilder_loadLightFaceData(Color* faceColor,
                                        std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}