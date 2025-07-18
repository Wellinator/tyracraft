#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"
#include "managers/liquid_helper.hpp"
#include "models/liquid_quad_map_model.hpp"

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

void WaterMeshBuilder_GenerateMesh(const Vec4* offset, const u8 visibleFaces,
                                   std::vector<Vec4>* t_vertices,
                                   std::vector<Color>* t_vertices_colors,
                                   std::vector<Vec4>* t_uv_map,
                                   WorldLightModel* t_worldLightModel,
                                   Level* pLevel);

void WaterMeshBuilder_loadMeshData(const Vec4* offset, const u8 visibleFaces,
                                   std::vector<Vec4>* t_vertices,
                                   Level* pLevel);

/**
 * https://minecraft.fandom.com/wiki/Water
 * 1	block	  1
 * 2	blocks	0.75-1
 * 3	blocks	0.625-0.75
 * 4	blocks	0.5-0.625
 * 5	blocks	0.375-0.5
 * 6	blocks	0.25-0.375
 * 7	blocks	0.125-0.25
 */
void WaterMeshBuilder_loadMeshDataByLevel(const Vec4* offset,
                                          const u8 visibleFaces,
                                          std::vector<Vec4>* t_vertices,
                                          const LiquidQuadMapModel quadMap);

void WaterMeshBuilder_loadUVData(const Vec4* offset, const u8 visibleFaces,
                                 std::vector<Vec4>* t_uv_map, Level* pLevel);
void WaterMeshBuilder_loadUVFaceData(const u8& index,
                                     std::vector<Vec4>* t_uv_map);
void WaterMeshBuilder_loadLightData(const Vec4* offset, const u8 visibleFaces,
                                    std::vector<Color>* t_vertices_colors,
                                    WorldLightModel* t_worldLightModel,
                                    Level* pLevel);

void WaterMeshBuilder_loadLightFaceData(Color* faceColor,
                                        std::vector<Color>* t_vertices_colors);
void WaterMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors);

bool WaterMeshBuilder_isBlockOpaque(u8 block_type);
std::array<u8, 8> WaterMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                     const Vec4* offset,
                                                     Level* pLevel);
