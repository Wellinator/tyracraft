#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include <array>
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

void CuboidMeshBuilder_GenerateMesh(
    const Vec4* offset, const u8 visibleFaces, const int lod,
    std::vector<Vec4>* t_vertices, std::vector<Color>* t_vertices_colors,
    std::vector<Vec4>* t_uv_map, WorldLightModel* t_worldLightModel,
    Level* pLevel, CustomMeshOptions* options = nullptr);

void CuboidMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                    const int lod,
                                    std::vector<Vec4>* t_vertices,
                                    CustomMeshOptions* options = nullptr);
void CuboidMeshBuilder_loadUVData(const Vec4* offset, const u8 visibleFaces,
                                  std::vector<Vec4>* t_uv_map);
void CuboidMeshBuilder_loadUVFaceData(const u8& index,
                                      std::vector<Vec4>* t_uv_map);
void CuboidMeshBuilder_loadLightData(const Vec4* offset, const u8 visibleFaces,
                                     std::vector<Color>* t_vertices_colors,
                                     WorldLightModel* t_worldLightModel,
                                     Level* pLevel);

// Pre-compute UV cache (call once at init)
void CuboidMeshBuilder_InitUVCache();

/**
 * @brief Return an array with the correct face by rotation
 * This function basicaly reverse the getBlockVisibleFaces orientation for
 * correct lighting face
 * This is the sequence of the array: [ left, front, back, right]
 *
 */
std::array<FACE_SIDE, 4> CuboidMeshBuilder_getFaceByRotation(const Vec4* offset,
                                                             Level* pLevel);

void CuboidMeshBuilder_loadLightFaceData(Color* faceColor,
                                         std::vector<Color>* t_vertices_colors);
void CuboidMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors);

bool CuboidMeshBuilder_isBlockOpaque(u8 block_type);
std::array<u8, 8> CuboidMeshBuilder_getFaceNeighbors(FACE_SIDE faceSide,
                                                     const Vec4* offset,
                                                     Level* pLevel);
