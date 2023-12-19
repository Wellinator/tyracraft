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
using Tyra::M4x4;
using Tyra::Vec4;

void TorchHandItemMeshBuilder_GenerateMesh(
    Block* t_block, std::vector<Vec4>* t_vertices,
    std::vector<Color>* t_vertices_colors, std::vector<Vec4>* t_uv_map,
    WorldLightModel* t_worldLightModel);

void TorchHandItemMeshBuilder_loadMeshData(Block* t_block,
                                           std::vector<Vec4>* t_vertices);
void TorchHandItemMeshBuilder_loadUVData(std::vector<Vec4>* t_uv_map);

void TorchHandItemMeshBuilder_loadLightData(
    Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel);

void TorchHandItemMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors);