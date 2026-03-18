#include "managers/clipping_manager.hpp"
#include "debug.hpp"

// Thread-local static buffers to avoid constant reallocations and dangling pointers
static std::vector<Vec4> g_clipped_vertices;
static std::vector<Vec4> g_clipped_uvs;
static std::vector<Color> g_clipped_colors;

int ClippingManager_ClipMesh(const u32 vertexCount, Vec4* in_vertex,
                             const u32 uvCount, Vec4* in_uv,
                             const u32 colorCount, Color* in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camLooksAt) {
  // Validation
  if (vertexCount == 0 || in_vertex == nullptr) return 0;

  Plane* frustumPlanes = (Plane*)t_renderer->core.renderer3D.frustumPlanes.getAll();

  const bool hasValidUVs = (uvCount > 0 && in_uv != nullptr);
  const bool hasValidColors = (colorCount > 0 && in_colors != nullptr);

  EEClipAlgorithmSettings algoSettings;
  algoSettings = {false, hasValidUVs, hasValidColors};

  static CustomPlanesClipAlgorithm algorithm;
  static std::vector<PlanesClipVertex> clippedTriangle;
  clippedTriangle.reserve(9);

  const u32 oldSize = out_vertex.size();
  const u32 triangleCount = vertexCount / 3;
  
  Vec4* colorsVec4 = reinterpret_cast<Vec4*>(in_colors);
  static Vec4 dummyVec4(0.0F, 0.0F, 0.0F, 1.0F);
  static Color dummyColor(0.0F, 0.0F, 0.0F, 128.0F);

  // Pre-resize to avoid capacity checks inside the loop
  const u32 maxPossibleVertices = triangleCount * 9; 
  out_vertex.resize(oldSize + maxPossibleVertices);
  if (hasValidUVs) out_uv.resize(oldSize + maxPossibleVertices);
  if (hasValidColors) out_colors.resize(oldSize + maxPossibleVertices);

  Vec4* outVPtr = out_vertex.data() + oldSize;
  Vec4* outUVPtr = out_uv.data() + oldSize;
  Color* outCPtr = out_colors.data() + oldSize;

  u32 totalClippedVertices = 0;
  std::array<PlanesClipVertexPtrs, 3> inputTriangle;

  for (u32 i = 0; i < triangleCount; i++) {
    const u32 baseIdx = i * 3;
    const Vec4& v0 = in_vertex[baseIdx];
    const Vec4& v1 = in_vertex[baseIdx + 1];
    const Vec4& v2 = in_vertex[baseIdx + 2];

    // Check triangle visibility and clipping needs
    Tyra::CoreBBoxFrustum frustumResult = Utils::FrustumTriangleIntersect(frustumPlanes, v0, v1, v2);

    if (frustumResult == Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM) {
      continue;
    }

    if (frustumResult == Tyra::CoreBBoxFrustum::IN_FRUSTUM) {
      *outVPtr++ = v0;
      *outVPtr++ = v1;
      *outVPtr++ = v2;
      
      if (hasValidUVs) {
        *outUVPtr++ = in_uv[baseIdx];
        *outUVPtr++ = in_uv[baseIdx + 1];
        *outUVPtr++ = in_uv[baseIdx + 2];
      } else {
        *outUVPtr++ = dummyVec4;
        *outUVPtr++ = dummyVec4;
        *outUVPtr++ = dummyVec4;
      }

      if (hasValidColors) {
        *outCPtr++ = in_colors[baseIdx];
        *outCPtr++ = in_colors[baseIdx + 1];
        *outCPtr++ = in_colors[baseIdx + 2];
      } else {
        *outCPtr++ = dummyColor;
        *outCPtr++ = dummyColor;
        *outCPtr++ = dummyColor;
      }
      totalClippedVertices += 3;
    } else {
      // Partial intersection: MUST clip against all 6 planes to prevent GS overflow on large quads
      inputTriangle[0].position = &in_vertex[baseIdx];
      inputTriangle[0].st = hasValidUVs ? &in_uv[baseIdx] : &dummyVec4;
      inputTriangle[0].color = hasValidColors ? &colorsVec4[baseIdx] : &dummyVec4;

      inputTriangle[1].position = &in_vertex[baseIdx + 1];
      inputTriangle[1].st = hasValidUVs ? &in_uv[baseIdx + 1] : &dummyVec4;
      inputTriangle[1].color = hasValidColors ? &colorsVec4[baseIdx + 1] : &dummyVec4;

      inputTriangle[2].position = &in_vertex[baseIdx + 2];
      inputTriangle[2].st = hasValidUVs ? &in_uv[baseIdx + 2] : &dummyVec4;
      inputTriangle[2].color = hasValidColors ? &colorsVec4[baseIdx + 2] : &dummyVec4;

      clippedTriangle.clear();
      int clippedCount = algorithm.clip(clippedTriangle, inputTriangle.data(), algoSettings, frustumPlanes);

      for (int j = 0; j < clippedCount; j++) {
        *outVPtr++ = clippedTriangle[j].position;
        *outUVPtr++ = clippedTriangle[j].st;
        *outCPtr++ = Color(clippedTriangle[j].color.xyzw);
      }
      totalClippedVertices += clippedCount;
    }
  }

  // Final resize to actual count
  out_vertex.resize(oldSize + totalClippedVertices);
  if (hasValidUVs) out_uv.resize(oldSize + totalClippedVertices);
  if (hasValidColors) out_colors.resize(oldSize + totalClippedVertices);

  return totalClippedVertices;
}

int ClippingManager_ClipMesh(std::vector<Vec4>& in_vertex,
                             std::vector<Vec4>& in_uv,
                             std::vector<Color>& in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camLooksAt) {
  return ClippingManager_ClipMesh(in_vertex.size(), in_vertex.data(),
                                  in_uv.size(), in_uv.data(), in_colors.size(),
                                  in_colors.data(), out_vertex, out_uv,
                                  out_colors, t_renderer, camLooksAt);
}

void ClippingManager_ClipAndRenderBag(StaPipBag* pBag, StaticPipeline* pStapip,
                                      Renderer* t_renderer, Vec4& camLooksAt) {
  const bool hasUV = pBag->texture != nullptr;
  const bool hasColors = pBag->color != nullptr && pBag->color->many != nullptr;

  g_clipped_vertices.clear();
  g_clipped_uvs.clear();
  g_clipped_colors.clear();

  const size_t reserveSize = pBag->count * 2 + 32; 
  g_clipped_vertices.reserve(reserveSize);
  g_clipped_uvs.reserve(reserveSize);
  g_clipped_colors.reserve(reserveSize);

  ClippingManager_ClipMesh(pBag->count, pBag->vertices,
                           hasUV ? pBag->count : 0, hasUV ? pBag->texture->coordinates : nullptr,
                           hasColors ? pBag->count : 0, hasColors ? const_cast<Color*>(pBag->color->many) : nullptr,
                           g_clipped_vertices, g_clipped_uvs, g_clipped_colors,
                           t_renderer, camLooksAt);

  pBag->count = g_clipped_vertices.size();
  if (pBag->count == 0) return;

  pBag->vertices = g_clipped_vertices.data();
  if (hasUV) pBag->texture->coordinates = g_clipped_uvs.data();
  if (hasColors) pBag->color->many = g_clipped_colors.data();

  pStapip->core.render(pBag);
}
