#pragma once

#include "tyra"
#include <tamtypes.h>
#include "managers/clipper/custom_planes_clip_algorithm.hpp"
#include "utils.hpp"

using Tyra::Color;
using Tyra::EEClipAlgorithmSettings;
using Tyra::Plane;
using Tyra::PlanesClipAlgorithm;
using Tyra::PlanesClipVertex;
using Tyra::PlanesClipVertexPtrs;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaticPipeline;
using Tyra::Vec4;

int ClippingManager_ClipMesh(const u32 vertexCount, Vec4* in_vertex,
                             const u32 uvCount, Vec4* in_uv,
                             const u32 colorCount, Color* in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camPos);

int ClippingManager_ClipMesh(std::vector<Vec4>& in_vertex,
                             std::vector<Vec4>& in_uv,
                             std::vector<Color>& in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camPos);

void ClippingManager_ClipAndRenderBag(StaticPipeline* pStapip, StaPipBag* pBag,
                                      Renderer* t_renderer, Vec4& camPos);
