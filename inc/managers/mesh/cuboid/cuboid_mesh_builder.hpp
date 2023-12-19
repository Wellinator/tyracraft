#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "models/world_light_model.hpp"
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
    Block* t_block, std::vector<Vec4>* t_vertices,
    std::vector<Color>* t_vertices_colors, std::vector<Vec4>* t_uv_map,
    WorldLightModel* t_worldLightModel, LevelMap* t_terrain);

void CuboidMeshBuilder_loadMeshData(Block* t_block,
                                    std::vector<Vec4>* t_vertices);
void CuboidMeshBuilder_loadUVData(Block* t_block, std::vector<Vec4>* t_uv_map);
void CuboidMeshBuilder_loadUVFaceData(const u8& index,
                                      std::vector<Vec4>* t_uv_map);
void CuboidMeshBuilder_loadLightData(Block* t_block,
                                     std::vector<Color>* t_vertices_colors,
                                     WorldLightModel* t_worldLightModel,
                                     LevelMap* t_terrain);

void CuboidMeshBuilder_loadLightFaceData(Color* faceColor,
                                         std::vector<Color>* t_vertices_colors);
void CuboidMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors);

bool CuboidMeshBuilder_isBlockOpaque(u8 block_type);
std::array<u8, 8> CuboidMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                Block* block,
                                                LevelMap* t_terrain);