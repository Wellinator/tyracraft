#pragma once

#include "constants.hpp"
#include "entities/item.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "models/world_light_model.hpp"
#include "entities/level.hpp"
#include "entities/Block.hpp"

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

void HandledItemMeshBuilder_BuildMesh(Item* t_item, Block* t_block,
                                      std::vector<Vec4>* t_vertices,
                                      std::vector<Color>* t_vertices_colors,
                                      std::vector<Vec4>* t_uv_map,
                                      WorldLightModel* t_worldLightModel);

void HandledItemMeshBuilder_BuildLightData(
    Item* t_item, Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel);
