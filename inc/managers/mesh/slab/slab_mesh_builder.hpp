#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "models/world_light_model.hpp"
#include "models/custom_mesh_options.hpp"
#include "entities/level.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;
using Tyra::Texture;
using Tyra::Vec4;

void SlabMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                  const int lod, std::vector<Vec4>* t_vertices,
                                  std::vector<Color>* t_vertices_colors,
                                  std::vector<Vec4>* t_uv_map,
                                  WorldLightModel* t_worldLightModel,
                                  Level* pLevel, CustomMeshOptions* options = nullptr);

void SlabMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                  const int lod, std::vector<Vec4>* t_vertices,
                                  Level* pLevel, CustomMeshOptions* options = nullptr);
void SlabMeshBuilder_loadUVData(const Vec4* offset, const u8 visibleFaces,
                                std::vector<Vec4>* t_uv_map);
void SlabMeshBuilder_loadSideUVFaceData(const u8& index,
                                        std::vector<Vec4>* t_uv_map);
void SlabMeshBuilder_loadTopDownUVFaceData(const u8& index,
                                           std::vector<Vec4>* t_uv_map);
void SlabMeshBuilder_loadLightData(const Vec4* offset, const u8 visibleFaces,
                                   std::vector<Color>* t_vertices_colors,
                                   WorldLightModel* t_worldLightModel,
                                   Level* pLevel);

/**
 * @brief Return an array with the correct face by rotation
 * This function basicaly reverse the getBlockVisibleFaces orientation for
 * correct lighting face
 * This is the sequence of the array: [ left, front, back, right]
 *
 */
std::array<FACE_SIDE, 4> SlabMeshBuilder_getFaceByRotation(const Vec4* offset,
                                                           Level* pLevel);

void SlabMeshBuilder_loadLightFaceData(Color* faceColor,
                                       std::vector<Color>* t_vertices_colors);
void SlabMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors);

bool SlabMeshBuilder_isBlockOpaque(u8 block_type);
std::array<u8, 8> SlabMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                    const Vec4* offset,
                                                    Level* pLevel);

void SlabMeshBuilder_ApplyLightToFace(Color* baseColor, const Vec4* offset,
                                      FACE_SIDE faceSide, Level* pLevel,
                                      const float sunlightIntensity);
