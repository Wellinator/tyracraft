#include "managers/clipping_manager.hpp"
#include "debug.hpp"

int ClippingManager_ClipMesh(const u32 vertexCount, Vec4* in_vertex,
                             const u32 uvCount, Vec4* in_uv,
                             const u32 colorCount, Color* in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camLooksAt) {
  // Route to the new clipping approach for consistency across entry points.
  return ClippingManager_CustomClipMesh(
      vertexCount, in_vertex, uvCount, in_uv, colorCount, in_colors, out_vertex,
      out_uv, out_colors, t_renderer, camLooksAt);
}

int ClippingManager_CustomClipMesh(const u32 vertexCount, Vec4* in_vertex,
                                   const u32 uvCount, Vec4* in_uv,
                                   const u32 colorCount, Color* in_colors,
                                   std::vector<Vec4>& out_vertex,
                                   std::vector<Vec4>& out_uv,
                                   std::vector<Color>& out_colors,
                                   Renderer* t_renderer, Vec4& camLooksAt) {
  // New clipping approach: Sutherland–Hodgman polygon clipping against all
  // 6 frustum planes with attribute interpolation (UVs, colors).
  // Input is a triangle list (3 vertices per triangle).

  Plane* frustumPlanes =
      (Plane*)t_renderer->core.renderer3D.frustumPlanes.getAll();

  const bool hasValidColors = colorCount > 0;
  const bool hasValidUVs = uvCount > 0;

  // Helper lambda: compute intersection point between segment (a->b) and plane
  // plane: normal n, distance d; distances da, db are signed distances to the
  // plane
  auto intersectPoint = [](const Vec4& a, const Vec4& b, const float da,
                           const float db) -> std::pair<Vec4, float> {
    // t = da / (da - db)
    float denom = (da - db);
    float t = denom != 0.0F ? (da / (da - db)) : 0.0F;
    Vec4 p = a + ((b - a) * t);
    p.w = 1.0F;
    return {p, t};
  };

  // Attribute lerp helper (Vec4)
  auto lerpV4 = [](const Vec4& a, const Vec4& b, float t) -> Vec4 {
    return Vec4::getByLerp(a, b, t);
  };

  // Clip a polygon (vector of PlanesClipVertex) against one plane
  auto clipAgainstPlane = [&](const Plane& plane,
                              const std::vector<PlanesClipVertex>& in,
                              std::vector<PlanesClipVertex>& out) {
    out.clear();
    if (in.empty()) return;

    size_t n = in.size();
    for (size_t i = 0; i < n; i++) {
      const PlanesClipVertex& cur = in[i];
      const PlanesClipVertex& nxt = in[(i + 1) % n];

      float dCur = plane.distanceTo(cur.position);
      float dNxt = plane.distanceTo(nxt.position);

      const bool curInside = dCur >= 0.0F;
      const bool nxtInside = dNxt >= 0.0F;

      if (curInside && nxtInside) {
        // Keep next
        out.push_back(nxt);
      } else if (curInside && !nxtInside) {
        // Leaving the volume: add intersection
        auto [pt, t] = intersectPoint(cur.position, nxt.position, dCur, dNxt);
        PlanesClipVertex v = {};
        v.position = pt;
        if (hasValidUVs) v.st = lerpV4(cur.st, nxt.st, t);
        if (hasValidColors) v.color = lerpV4(cur.color, nxt.color, t);
        out.push_back(v);
      } else if (!curInside && nxtInside) {
        // Entering the volume: add intersection and next
        auto [pt, t] = intersectPoint(cur.position, nxt.position, dCur, dNxt);
        PlanesClipVertex v = {};
        v.position = pt;
        if (hasValidUVs) v.st = lerpV4(cur.st, nxt.st, t);
        if (hasValidColors) v.color = lerpV4(cur.color, nxt.color, t);
        out.push_back(v);
        out.push_back(nxt);
      } else {
        // Both outside: emit nothing
      }
    }
  };

  // Working buffers for polygon clipping
  std::vector<PlanesClipVertex> polyIn;
  std::vector<PlanesClipVertex> polyOut;
  polyIn.reserve(16);
  polyOut.reserve(16);

  int emitted = 0;

  // Iterate triangles
  for (u32 i = 0; i + 2 < vertexCount; i += 3) {
    // Seed polygon with the triangle
    polyIn.clear();
    for (u8 k = 0; k < 3; k++) {
      PlanesClipVertex v = {};
      v.position = in_vertex[i + k];
      if (hasValidUVs && (i + k) < uvCount) v.st = in_uv[i + k];
      if (hasValidColors && (i + k) < colorCount)
        v.color = reinterpret_cast<Vec4*>(in_colors)[i + k];
      polyIn.push_back(v);
    }

    // Back-face culling (approximate):
    // We treat camLooksAt as the camera position proxy here.
    // If the triangle faces away from the camera, skip it early.
    if (Vec4::shouldBeBackfaceCulled(&camLooksAt, &polyIn[2].position,
                                     &polyIn[1].position, &polyIn[0].position))
      continue;

    // Early trivial reject using all 6 planes (AABB-like but per-vertex test)
    bool triviallyOutside = false;
    for (u8 p = 0; p < 6; ++p) {
      u8 insideCount = 0;
      for (const auto& v : polyIn)
        insideCount += (frustumPlanes[p].distanceTo(v.position) >= 0.0F);
      if (insideCount == 0) {
        triviallyOutside = true;
        break;
      }
    }
    if (triviallyOutside) continue;

    // Clip against 6 planes: L, R, B, T, N, F
    for (u8 p = 0; p < 6; ++p) {
      clipAgainstPlane(frustumPlanes[p], polyIn, polyOut);
      polyIn.swap(polyOut);
      if (polyIn.empty()) break;
    }

    if (polyIn.size() < 3) continue;

    // Triangulate fan and emit
    const PlanesClipVertex v0 = polyIn[0];
    for (size_t k = 1; k + 1 < polyIn.size(); ++k) {
      const PlanesClipVertex& v1 = polyIn[k];
      const PlanesClipVertex& v2 = polyIn[k + 1];

      out_vertex.emplace_back(v0.position);
      out_vertex.emplace_back(v1.position);
      out_vertex.emplace_back(v2.position);

      if (hasValidUVs) {
        out_uv.emplace_back(v0.st);
        out_uv.emplace_back(v1.st);
        out_uv.emplace_back(v2.st);
      }

      if (hasValidColors) {
        out_colors.emplace_back(v0.color.xyzw);
        out_colors.emplace_back(v1.color.xyzw);
        out_colors.emplace_back(v2.color.xyzw);
      }

      emitted += 3;
    }
  }

  return emitted;
}

int ClippingManager_ClipMesh(std::vector<Vec4>& in_vertex,
                             std::vector<Vec4>& in_uv,
                             std::vector<Color>& in_colors,
                             std::vector<Vec4>& out_vertex,
                             std::vector<Vec4>& out_uv,
                             std::vector<Color>& out_colors,
                             Renderer* t_renderer, Vec4& camLooksAt) {
  return ClippingManager_CustomClipMesh(
      in_vertex.size(), in_vertex.data(), in_uv.size(), in_uv.data(),
      in_colors.size(), in_colors.data(), out_vertex, out_uv, out_colors,
      t_renderer, camLooksAt);
}

void ClippingManager_ClipAndRenderBag(StaPipBag* pBag, StaticPipeline* pStapip,
                                      Renderer* t_renderer, Vec4& camLooksAt) {
  const bool hasUV = pBag->texture != nullptr;
  const bool hasColors = pBag->color != nullptr && pBag->color->many != nullptr;

  std::vector<Vec4> out_vertex, out_uv;
  std::vector<Color> out_colors;

  out_vertex.clear();
  out_uv.clear();
  out_colors.clear();

  out_vertex.reserve(pBag->count);
  if (hasUV) out_uv.reserve(pBag->count);
  if (hasColors) out_colors.reserve(pBag->count);

  ClippingManager_ClipMesh(
      pBag->count, pBag->vertices,

      // Check if the bag contains texture data (UV)
      hasUV ? pBag->count : 0, pBag->texture->coordinates,

      // Check if the bag contains color data
      hasColors ? pBag->count : 0, const_cast<Color*>(pBag->color->many),

      out_vertex, out_uv, out_colors, t_renderer, camLooksAt);

  pBag->count = out_vertex.size();
  pBag->vertices = out_vertex.data();

  if (hasUV) {
    pBag->texture->coordinates = out_uv.data();
  }

  if (hasColors) {
    pBag->color->many = out_colors.data();
  }

  pStapip->core.render(pBag);
}
