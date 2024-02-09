#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "models/world_light_model.hpp"
#include "models/liquid_quad_map_model.hpp"
#include "entities/level.hpp"
#include "managers/liquid_helper.hpp"

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

void LavaMeshBuilder_GenerateMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                                  std::vector<Color>* t_vertices_colors,
                                  std::vector<Vec4>* t_uv_map,
                                  WorldLightModel* t_worldLightModel,
                                  LevelMap* t_terrain);

void LavaMeshBuilder_loadMeshData(Block* t_block, std::vector<Vec4>* t_vertices,
                                  LevelMap* t_terrain);

/**
 * https://minecraft.fandom.com/wiki/Water
 * 1	block	1
 * 2	blocks	0.75-1
 * 3	blocks	0.5-0.75
 * 4	blocks	0.25-0.5
 */
void LavaMeshBuilder_loadMeshDataByLevel(Block* t_block,
                                         std::vector<Vec4>* t_vertices,
                                         const LiquidOrientation orientation,
                                         const LiquidQuadMapModel quadMap);

void LavaMeshBuilder_loadUVData(Block* t_block, std::vector<Vec4>* t_uv_map);
void LavaMeshBuilder_loadUVFaceData(const u8& index,
                                    std::vector<Vec4>* t_uv_map);
void LavaMeshBuilder_loadLightData(Block* t_block,
                                   std::vector<Color>* t_vertices_colors,
                                   WorldLightModel* t_worldLightModel,
                                   LevelMap* t_terrain);

void LavaMeshBuilder_loadLightFaceData(Color* faceColor,
                                       std::vector<Color>* t_vertices_colors);
void LavaMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors);

bool LavaMeshBuilder_isBlockOpaque(u8 block_type);
std::array<u8, 8> LavaMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                    Block* block,
                                                    LevelMap* t_terrain);
