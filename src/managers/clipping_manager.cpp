#include "managers/clipping_manager.hpp"

int ClippingManager_ClipMesh(const u32 vertexCount, Vec4* in_vertex,
                             const u32 uvCount, Vec4* in_uv,
                             const u32 colorCount, Color* in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camPos) {
  Plane* frustumPlanes =
      (Plane*)t_renderer->core.renderer3D.frustumPlanes.getAll();

  EEClipAlgorithmSettings algoSettings;
  algoSettings = {false, uvCount > 0, colorCount > 0};

  CustomPlanesClipAlgorithm algorithm;

  int result = 0;

  Vec4 inputVerts[3];
  std::array<PlanesClipVertexPtrs, 3> inputTriangle;

  std::vector<PlanesClipVertex> clippedTriangle;
  std::vector<PlanesClipVertex> clippedVertices;
  // clippedVertices.reserve(9);

  Vec4* vert = in_vertex;
  Vec4* sts = in_uv;
  Vec4* colors = reinterpret_cast<Vec4*>(in_colors);

  // Iterate over the input vertices per triangles
  for (u32 i = 0; i < vertexCount / 3; i++) {
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

    if (
        // Back face culling
        !Vec4::shouldBeBackfaceCulled(&camPos, &inputVerts[0], &inputVerts[1],
                                      &inputVerts[2]) ||

        // Check if the triangle is partially visible
        Utils::FrustumTriangleIntersect(frustumPlanes, inputVerts[0],
                                        inputVerts[1], inputVerts[2]) !=
            Tyra::CoreBBoxFrustum::PARTIALLY_IN_FRUSTUM) {
      continue;
    }

    int clippedVertivesCount = algorithm.clip(
        clippedTriangle, inputTriangle.data(), algoSettings, frustumPlanes);

    if (clippedVertivesCount == 0) {
      continue;
    }

    result += clippedVertivesCount;

    for (size_t j = 0; j < clippedTriangle.size(); j++) {
      clippedVertices.emplace_back(clippedTriangle[j]);
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

int ClippingManager_ClipMesh(std::vector<Vec4>& in_vertex,
                             std::vector<Vec4>& in_uv,
                             std::vector<Color>& in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camPos) {
  return ClippingManager_ClipMesh(in_vertex.size(), in_vertex.data(),
                                  in_uv.size(), in_uv.data(), in_colors.size(),
                                  in_colors.data(), out_vertex, out_uv,
                                  out_colors, t_renderer, camPos);
}

void ClippingManager_ClipAndRenderBag(StaticPipeline* pStapip, StaPipBag* pBag,
                                      Renderer* t_renderer, Vec4& camPos) {
  const bool hasUV = pBag->texture != nullptr;
  const bool hasColors = pBag->color != nullptr && pBag->color->many != nullptr;

  // std::vector<Vec4> out_vertex(pBag->vertices, pBag->vertices + pBag->count);
  // std::vector<Vec4> out_uv(pBag->texture->coordinates,
  //                          pBag->texture->coordinates + pBag->count);
  // std::vector<Color> out_colors(pBag->color->many,
  //                               pBag->color->many + pBag->count);

  std::vector<Vec4> out_vertex, out_uv;
  std::vector<Color> out_colors;

  out_vertex.reserve(pBag->count);
  if (hasUV) out_uv.reserve(pBag->count);
  if (hasColors) out_colors.reserve(pBag->count);

  // out_vertex(pBag->vertices, pBag->vertices + pBag->count);
  // if (hasColors) out_colors(pBag->color->many, pBag->color->many +
  // pBag->count); if (hasUV)
  //   out_uv(pBag->texture->coordinates,
  //          pBag->texture->coordinates + pBag->count);

  int generatedVertexCounter = ClippingManager_ClipMesh(
      pBag->count, pBag->vertices,

      // Check if the bag contains texture data (UV)
      hasUV ? pBag->count : 0, pBag->texture->coordinates,

      // Check if the bag contains color data
      hasColors ? pBag->count : 0, const_cast<Color*>(pBag->color->many),

      out_vertex, out_uv, out_colors, t_renderer, camPos);

  if (generatedVertexCounter > 0) {
    pBag->vertices = out_vertex.data();
    if (hasUV) pBag->texture->coordinates = out_uv.data();
    if (hasColors) {
      StaPipColorBag newColorBag;
      newColorBag.many = out_colors.data();
      pBag->color = &newColorBag;
    };
  }

  pStapip->core.render(pBag);
}
