#pragma once

#include "tyra"
#include <tamtypes.h>
#include "managers/clipper/custom_planes_clip_algorithm.hpp"

using Tyra::Color;
using Tyra::EEClipAlgorithmSettings;
using Tyra::Plane;
using Tyra::PlanesClipAlgorithm;
using Tyra::PlanesClipVertex;
using Tyra::PlanesClipVertexPtrs;
using Tyra::RendererSettings;
using Tyra::Vec4;

/**
 * @brief Clip mesh data
 *
 * @param in_vertex
 * @param in_uv
 * @param in_colors
 * @return int
 */
int ClippingManager_ClipMesh(std::vector<Vec4>& in_vertex,
                             std::vector<Vec4>& in_uv,
                             std::vector<Color>& in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camPos) {
  Plane* frustumPlanes =
      (Plane*)t_renderer->core.renderer3D.frustumPlanes.getAll();

  EEClipAlgorithmSettings algoSettings;
  algoSettings = {false, in_uv.size() > 0, in_colors.size() > 0};

  CustomPlanesClipAlgorithm algorithm;

  int result = 0;

  Vec4 inputVerts[3];
  std::array<PlanesClipVertexPtrs, 3> inputTriangle;

  std::vector<PlanesClipVertex> clippedTriangle;
  std::vector<PlanesClipVertex> clippedVertices;
  // clippedVertices.reserve(9);

  Vec4* vert = in_vertex.data();
  Vec4* sts = in_uv.data();
  Vec4* colors = reinterpret_cast<Vec4*>(in_colors.data());

  // Iterate over the input vertices per triangles
  for (u32 i = 0; i < in_vertex.size() / 3; i++) {
    // Iterate over the triangles
    for (u8 j = 0; j < 3; j++) {
      inputVerts[j] = vert[i * 3 + j];

      inputTriangle[j] = {&inputVerts[j],

                          // Normals used with dir light
                          nullptr,

                          // UV
                          &sts[i * 3 + j],

                          // Colors
                          &colors[i * 3 + j]};
    }

    if (!Vec4::shouldBeBackfaceCulled(&camPos, &inputVerts[0], &inputVerts[1],
                                     &inputVerts[2])) {
      continue;
    }

    int clippedVertivesCount = algorithm.clip(
        clippedTriangle, inputTriangle.data(), algoSettings, frustumPlanes);

    if (clippedVertivesCount == 0) {
      continue;
    }

    result += clippedVertivesCount;

    for (size_t j = 0; j < clippedTriangle.size(); j++) {
      clippedVertices.push_back(clippedTriangle[j]);
    }
  }

  if (clippedVertices.size() > 0) {
    out_vertex.reserve(clippedVertices.size());
    out_uv.reserve(clippedVertices.size());
    out_colors.reserve(clippedVertices.size());

    for (u32 i = 0; i < clippedVertices.size(); i++) {
      out_vertex.emplace_back(clippedVertices[i].position);
      out_uv.emplace_back(clippedVertices[i].st);
      out_colors.emplace_back(clippedVertices[i].color.xyzw);
    }
  }

  return result;
}
