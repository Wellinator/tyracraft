#include "managers/mesh/lava/lava_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include <algorithm>

void LavaMeshBuilder_GenerateMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                                  std::vector<Color>* t_vertices_colors,
                                  std::vector<Vec4>* t_uv_map,
                                  WorldLightModel* t_worldLightModel,
                                  Level* pLevel) {
  LavaMeshBuilder_loadMeshData(t_block, t_vertices, pLevel);
  LavaMeshBuilder_loadUVData(t_block, t_uv_map);
  LavaMeshBuilder_loadLightData(t_block, t_vertices_colors, t_worldLightModel,
                                pLevel);
}

void LavaMeshBuilder_loadMeshData(Block* t_block, std::vector<Vec4>* t_vertices,
                                  Level* pLevel) {
  Vec4 pos;
  pLevel->GetXYZFromPos(&t_block->offset, &pos);

  const LiquidOrientation orientation =
      pLevel->GetLiquidOrientationDataFromMap(pos.x, pos.y, pos.z);

  const LiquidQuadMapModel quadMap = LiquidHelper_getQuadMap(
      pLevel, orientation, &pos, (u8)Blocks::LAVA_BLOCK);

  LavaMeshBuilder_loadMeshDataByLevel(t_block, t_vertices, orientation,
                                      quadMap);
}

void LavaMeshBuilder_loadMeshDataByLevel(Block* t_block,
                                         std::vector<Vec4>* t_vertices,
                                         const LiquidOrientation orientation,
                                         const LiquidQuadMapModel quadMap) {
  u8 vert = 0;
  const Vec4* rawData = VertexBlockData::cuboidVertexData;

  Vec4 modelNW = Vec4(0.0F, quadMap.NW, 0.0F);
  Vec4 modelNE = Vec4(0.0F, quadMap.NE, 0.0F);
  Vec4 modelSE = Vec4(0.0F, quadMap.SE, 0.0F);
  Vec4 modelSW = Vec4(0.0F, quadMap.SW, 0.0F);

  if (t_block->isTopFaceVisible()) {
    vert = 0;
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNE);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSE);

    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNW);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNE);
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
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);

    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNW);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSW);
  }
  if (t_block->isRightFaceVisible()) {
    vert = 18;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNE);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);

    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSE);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNE);
  }
  if (t_block->isBackFaceVisible()) {
    vert = 24;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNW);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);

    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNE);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelNW);
  }
  if (t_block->isFrontFaceVisible()) {
    vert = 30;
    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSE);
    t_vertices->emplace_back(t_block->model * rawData[vert++]);

    t_vertices->emplace_back(t_block->model * rawData[vert++]);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSW);
    t_vertices->emplace_back(t_block->model * rawData[vert++] - modelSE);
  }
}

void LavaMeshBuilder_loadUVData(Block* t_block, std::vector<Vec4>* t_uv_map) {
  u8* facesMap = t_block->getFacesMap().data();

  if (t_block->isTopFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(facesMap[0], t_uv_map);
  }
  if (t_block->isBottomFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(facesMap[1], t_uv_map);
  }
  if (t_block->isLeftFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(facesMap[2], t_uv_map);
  }
  if (t_block->isRightFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(facesMap[3], t_uv_map);
  }
  if (t_block->isBackFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(facesMap[4], t_uv_map);
  }
  if (t_block->isFrontFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(facesMap[5], t_uv_map);
  }
}

void LavaMeshBuilder_loadUVFaceData(const u8& index,
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

void LavaMeshBuilder_loadLightData(Block* t_block,
                                   std::vector<Color>* t_vertices_colors,
                                   WorldLightModel* t_worldLightModel,
                                   Level* pLevel) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  if (t_block->isTopFaceVisible()) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP, pLevel,
                                   t_worldLightModel->sunLightIntensity);

    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isBottomFaceVisible()) {
    //   Top face 50% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.5F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BOTTOM,
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isLeftFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::LEFT, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isRightFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::RIGHT,
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isBackFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BACK, pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isFrontFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::FRONT,
                                   pLevel,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  blockColorAverage /= t_block->packed.visibleFacesCount;
  t_block->baseColor.set(blockColorAverage.x, blockColorAverage.y,
                         blockColorAverage.z);
}

void LavaMeshBuilder_loadLightFaceData(Color* faceColor,
                                       std::vector<Color>* t_vertices_colors) {
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);

  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
  t_vertices_colors->emplace_back(*faceColor);
}