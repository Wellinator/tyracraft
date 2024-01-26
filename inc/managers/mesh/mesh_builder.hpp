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

void MeshBuilder_RegisterBuilders();
void MeshBuilder_UnregisterBuilders();

void MeshBuilder_BuildMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                           std::vector<Color>* t_vertices_colors,
                           std::vector<Vec4>* t_uv_map,
                           WorldLightModel* t_worldLightModel,
                           LevelMap* t_terrain);

void MeshBuilder_BuildLightData(Block* t_block,
                                std::vector<Color>* t_vertices_colors,
                                WorldLightModel* t_worldLightModel,
                                LevelMap* t_terrain);
