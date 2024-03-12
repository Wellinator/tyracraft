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

void SlabMeshBuilder_GenerateMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                                  std::vector<Color>* t_vertices_colors,
                                  std::vector<Vec4>* t_uv_map,
                                  WorldLightModel* t_worldLightModel,
                                  LevelMap* t_terrain);

void SlabMeshBuilder_loadMeshData(Block* t_block, std::vector<Vec4>* t_vertices,
                                  LevelMap* t_terrain);
void SlabMeshBuilder_loadUVData(Block* t_block, std::vector<Vec4>* t_uv_map);
void SlabMeshBuilder_loadSideUVFaceData(const u8& index,
                                        std::vector<Vec4>* t_uv_map);
void SlabMeshBuilder_loadTopDownUVFaceData(const u8& index,
                                           std::vector<Vec4>* t_uv_map);
void SlabMeshBuilder_loadLightData(Block* t_block,
                                   std::vector<Color>* t_vertices_colors,
                                   WorldLightModel* t_worldLightModel,
                                   LevelMap* t_terrain);

void SlabMeshBuilder_loadLightFaceData(Color* faceColor,
                                       std::vector<Color>* t_vertices_colors);
void SlabMeshBuilder_loadLightFaceDataWithAO(
    Color* faceColor, std::array<u8, 8>& faceNeightbors,
    std::vector<Color>* t_vertices_colors);

bool SlabMeshBuilder_isBlockOpaque(u8 block_type);
std::array<u8, 8> SlabMeshBuilder_getFaceNeightbors(FACE_SIDE faceSide,
                                                    Block* block,
                                                    LevelMap* t_terrain);

void SlabMeshBuilder_ApplyLightToFace(Color* baseColor, Block* targetBlock,
                                      FACE_SIDE faceSide, LevelMap* t_terrain,
                                      const float sunlightIntensity);
