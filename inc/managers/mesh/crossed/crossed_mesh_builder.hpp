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

void CrossedMeshBuilder_GenerateMesh(
    const Vec4* offset, const u8 visibleFaces, const int lod,
    std::vector<Vec4>* t_vertices, std::vector<Color>* t_vertices_colors,
    std::vector<Vec4>* t_uv_map, WorldLightModel* t_worldLightModel,
    Level* pLevel, CustomMeshOptions* options = nullptr);

void CrossedMeshBuilder_loadCrossedMeshData(
    const Vec4* offset, std::vector<Vec4>* t_vertices,
    CustomMeshOptions* options = nullptr);

void CrossedMeshBuilder_loadCrossedUVData(const Vec4* offset,
                                          std::vector<Vec4>* t_uv_map);
void CrossedMeshBuilder_loadUVFaceData(const u8& index,
                                       std::vector<Vec4>* t_uv_map);
void CrossedMeshBuilder_loadCrossedLightData(
    const Vec4* offset, const u8 visibleFaces,
    std::vector<Color>* t_vertices_colors, WorldLightModel* t_worldLightModel,
    Level* pLevel);

void CrossedMeshBuilder_loadLightFaceData(
    Color* faceColor, std::vector<Color>* t_vertices_colors);