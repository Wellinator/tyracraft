#include "managers/mesh/lava/lava_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include <algorithm>

void LavaMeshBuilder_GenerateMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                                  std::vector<Color>* t_vertices_colors,
                                  std::vector<Vec4>* t_uv_map,
                                  WorldLightModel* t_worldLightModel,
                                  LevelMap* t_terrain) {
  LavaMeshBuilder_loadMeshData(t_block, t_vertices, t_terrain);
  LavaMeshBuilder_loadUVData(t_block, t_uv_map);
  LavaMeshBuilder_loadLightData(t_block, t_vertices_colors, t_worldLightModel,
                                t_terrain);
}

void LavaMeshBuilder_loadMeshData(Block* t_block, std::vector<Vec4>* t_vertices,
                                  LevelMap* t_terrain) {
  Vec4 pos;
  GetXYZFromPos(&t_block->offset, &pos);

  const LiquidOrientation orientation =
      GetLiquidOrientationDataFromMap(t_terrain, pos.x, pos.y, pos.z);

  const LiquidQuadMapModel quadMap = LiquidHelper_getQuadMap(
      t_terrain, orientation, &pos, (u8)Blocks::LAVA_BLOCK);

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
  if (t_block->isTopFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(t_block->facesMapIndex[0], t_uv_map);
  }
  if (t_block->isBottomFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(t_block->facesMapIndex[1], t_uv_map);
  }
  if (t_block->isLeftFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(t_block->facesMapIndex[2], t_uv_map);
  }
  if (t_block->isRightFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(t_block->facesMapIndex[3], t_uv_map);
  }
  if (t_block->isBackFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(t_block->facesMapIndex[4], t_uv_map);
  }
  if (t_block->isFrontFaceVisible()) {
    LavaMeshBuilder_loadUVFaceData(t_block->facesMapIndex[5], t_uv_map);
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
                                   LevelMap* t_terrain) {
  auto baseFaceColor = Color(120, 120, 120);
  Vec4 blockColorAverage = Vec4(0.0F);
  Vec4 tempColor;

  if (t_block->isTopFaceVisible()) {
    //   Top face 100% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 1.0F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::TOP,
                                   t_terrain,
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
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isLeftFaceVisible()) {
    // X-side faces 60% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.6F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::LEFT,
                                   t_terrain,
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
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  if (t_block->isBackFaceVisible()) {
    // Z-side faces 80% of the base color
    Color faceColor = LightManager::IntensifyColor(&baseFaceColor, 0.8F);

    // Apply sunlight and block light to face
    LightManager::ApplyLightToFace(&faceColor, t_block, FACE_SIDE::BACK,
                                   t_terrain,
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
                                   t_terrain,
                                   t_worldLightModel->sunLightIntensity);
    Vec4::copy(&tempColor, faceColor.rgba);
    blockColorAverage += tempColor;

    LavaMeshBuilder_loadLightFaceData(&faceColor, t_vertices_colors);
  }

  blockColorAverage /= t_block->visibleFacesCount;
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