#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"
#include "entities/item.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::Vec4;

void SlabHandItemMeshBuilder_GenerateMesh(Block* t_block,
                                          std::vector<Vec4>* t_vertices,
                                          std::vector<Color>* t_vertices_colors,
                                          std::vector<Vec4>* t_uv_map,
                                          WorldLightModel* t_worldLightModel);

void SlabHandItemMeshBuilder_loadMeshData(Block* t_block,
                                          std::vector<Vec4>* t_vertices);
void SlabHandItemMeshBuilder_loadUVData(Block* t_block,
                                        std::vector<Vec4>* t_uv_map);
void SlabHandItemMeshBuilder_loadSideUVFaceData(const u8& index,
                                            std::vector<Vec4>* t_uv_map);
void SlabHandItemMeshBuilder_loadTopDownUVFaceData(const u8& index,
                                            std::vector<Vec4>* t_uv_map);
void SlabHandItemMeshBuilder_loadLightData(
    Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel);

void SlabHandItemMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors);
