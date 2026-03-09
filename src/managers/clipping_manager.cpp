#include "managers/clipping_manager.hpp"
#include "debug.hpp"

// Thread-local static buffers para evitar realocações constantes e dangling
// pointers
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
#ifdef DEBUG_MODE
  if (g_debug_menu.noclipMode) {
    out_vertex.clear();
    out_uv.clear();
    out_colors.clear();

    out_vertex.reserve(vertexCount);
    out_uv.reserve(uvCount);
    out_colors.reserve(colorCount);

    for (u32 i = 0; i < vertexCount; i++) {
      out_vertex.emplace_back(in_vertex[i]);
    }
    for (u32 i = 0; i < uvCount; i++) {
      out_uv.emplace_back(in_uv[i]);
    }
    for (u32 i = 0; i < colorCount; i++) {
      out_colors.emplace_back(in_colors[i]);
    }

    return vertexCount;
  }
#endif  // DEBUG_MODE

  // Validação de entrada
  if (vertexCount == 0 || in_vertex == nullptr) return 0;

  Plane* frustumPlanes =
      (Plane*)t_renderer->core.renderer3D.frustumPlanes.getAll();

  const bool hasValidUVs = (uvCount > 0 && in_uv != nullptr);
  const bool hasValidColors = (colorCount > 0 && in_colors != nullptr);

  EEClipAlgorithmSettings algoSettings;
  algoSettings = {false, hasValidUVs, hasValidColors};

  CustomPlanesClipAlgorithm algorithm;

  int result = 0;

  std::array<PlanesClipVertexPtrs, 3> inputTriangle;

  // Static buffers: allocated once, never freed — eliminates malloc/free per draw call on PS2
  static std::vector<PlanesClipVertex> clippedTriangle;
  static std::vector<PlanesClipVertex> clippedVertices;
  clippedTriangle.clear();
  clippedVertices.clear();
  // Pre-reserve on first use; subsequent frames reuse the same memory
  if (clippedTriangle.capacity() < 9) clippedTriangle.reserve(9);
  if (clippedVertices.capacity() < 128) clippedVertices.reserve(128);

  // Pre-cast colors para evitar cast repetido (seguro mesmo se nullptr)
  Vec4* colorsVec4 = reinterpret_cast<Vec4*>(in_colors);

  // Dummy data para quando não há UVs/cores (sem const para compatibilidade com
  // ponteiros)
  static Vec4 dummyVec4(0.0F, 0.0F, 0.0F, 1.0F);

  // Iterate over the input vertices per triangles
  const u32 triangleCount = vertexCount / 3;
  for (u32 i = 0; i < triangleCount; i++) {
    const u32 baseIdx = i * 3;

    // Setup input triangle usando ponteiros diretos (sem cópias)
    for (u8 j = 0; j < 3; j++) {
      const u32 idx = baseIdx + j;

      inputTriangle[j].position = &in_vertex[idx];
      inputTriangle[j].normal = nullptr;  // Normals (não usado)
      inputTriangle[j].st = hasValidUVs ? &in_uv[idx] : &dummyVec4;
      inputTriangle[j].color = hasValidColors ? &colorsVec4[idx] : &dummyVec4;
    }

    // // Software backface culling — skip triangles that face away from the camera.
    // // Convention: shouldBeBackfaceCulled(cam, v2, v1, v0) computes the face
    // // normal as (v1-v2)×(v0-v2) and returns true when dot(normal, cam-v2) ≤ 0.
    // // All mesh builders (CuboidMeshBuilder and BinaryGreedyMesher) must emit
    // // triangles with winding that produces an outward normal under this formula.
    // if (Vec4::shouldBeBackfaceCulled(&camLooksAt, in_vertex + baseIdx + 2,
    //                                  in_vertex + baseIdx + 1,
    //                                  in_vertex + baseIdx + 0)) {
    //   continue;
    // }

    // Check triangle visibility and clipping needs
    CoreBBoxFrustum frustumResult = Utils::FrustumTriangleIntersect(
        frustumPlanes, in_vertex[baseIdx], in_vertex[baseIdx + 1],
        in_vertex[baseIdx + 2]);

    // Triangle is completely outside frustum
    if (frustumResult == Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM) {
      continue;
    }

    int clippedVerticesCount = 0;

    if (frustumResult == Tyra::CoreBBoxFrustum::IN_FRUSTUM) {
      // Triangle is completely inside frustum, no clipping needed
      // Adicionar diretamente ao output para evitar cópias extras
      clippedTriangle.clear();
      for (u8 j = 0; j < 3; j++) {
        const u32 idx = baseIdx + j;
        PlanesClipVertex v;
        v.position = in_vertex[idx];
        v.st = hasValidUVs ? in_uv[idx] : dummyVec4;
        v.color = hasValidColors ? colorsVec4[idx] : dummyVec4;
        clippedTriangle.push_back(v);
      }
      clippedVerticesCount = 3;
    } else {
      // Triangle is partially in frustum, needs clipping
      clippedVerticesCount = algorithm.clip(
          clippedTriangle, inputTriangle.data(), algoSettings, frustumPlanes);
    }

    if (clippedVerticesCount == 0) {
      continue;
    }

    result += clippedVerticesCount;

    // Copiar vértices clipped para o buffer final
    clippedVertices.insert(clippedVertices.end(), clippedTriangle.begin(),
                           clippedTriangle.end());
  }

  if (!clippedVertices.empty()) {
    const size_t numClipped = clippedVertices.size();
    // Note: out_vertex/uv/colors are the global static buffers pre-reserved by
    // ClippingManager_ClipAndRenderBag — do NOT reserve again here.
    for (size_t i = 0; i < numClipped; i++) {
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

  // Usar buffers estáticos thread-local para evitar dangling pointers
  // e reduzir alocações de memória
  g_clipped_vertices.clear();
  g_clipped_uvs.clear();
  g_clipped_colors.clear();

  // Reserve com espaço extra para clipping (pode gerar até 3x mais vértices)
  const size_t reserveSize = pBag->count * 3;
  g_clipped_vertices.reserve(reserveSize);
  if (hasUV) g_clipped_uvs.reserve(reserveSize);
  if (hasColors) g_clipped_colors.reserve(reserveSize);

  ClippingManager_ClipMesh(pBag->count, pBag->vertices,

                           // Check if the bag contains texture data (UV)
                           hasUV ? pBag->count : 0, pBag->texture->coordinates,

                           // Check if the bag contains color data
                           hasColors ? pBag->count : 0,
                           const_cast<Color*>(pBag->color->many),

                           g_clipped_vertices, g_clipped_uvs, g_clipped_colors,
                           t_renderer, camLooksAt);

  // Atualizar bag com dados clipped (agora seguros pois são estáticos)
  pBag->count = g_clipped_vertices.size();
  pBag->vertices = g_clipped_vertices.data();

  if (hasUV) {
    pBag->texture->coordinates = g_clipped_uvs.data();
  }

  if (hasColors) {
    pBag->color->many = g_clipped_colors.data();
  }

  pStapip->core.render(pBag);
}
