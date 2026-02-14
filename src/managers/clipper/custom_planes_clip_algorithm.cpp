/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2024, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Wellington Carvalho (Wellinator) <wellcoj@gmail.com>
*/

#include "managers/clipper/custom_planes_clip_algorithm.hpp"
#include <list>
#include <vector>

CustomPlanesClipAlgorithm::CustomPlanesClipAlgorithm() {}

CustomPlanesClipAlgorithm::~CustomPlanesClipAlgorithm() {}

Vec4 CustomPlanesClipAlgorithm::intersectPlane(Vec4& plane_n, Vec4& lineStart,
                                               Vec4& lineEnd, float plane_d,
                                               float& t) {
  float d1 = plane_d + plane_n.innerProduct(lineStart);
  float d2 = plane_d + plane_n.innerProduct(lineEnd);

  t = d1 / (d1 - d2);
  Vec4 result = lineStart + ((lineEnd - lineStart) * t);
  result.w = 1.0F;
  return result;
}

int CustomPlanesClipAlgorithm::clip(std::vector<PlanesClipVertex>& o_vertices,
                                    PlanesClipVertexPtrs* i_vertices,
                                    const EEClipAlgorithmSettings& settings,
                                    Plane* frustumPlanes) {
  // Use static arrays to avoid allocation overhead
  // A clipped triangle rarely exceeds 7-8 vertices, 24 is plenty safe.
  static PlanesClipVertex buffers[2][24];
  
  // Current input/output counts
  int inCount = 3;
  int outCount = 0;
  
  // Index of the current input buffer (0 or 1)
  int inIdx = 0;

  // 1. Fill initial buffer
  buffers[0][0].position = *i_vertices[0].position;
  buffers[0][0].st = settings.lerpTexCoords ? *i_vertices[0].st : Vec4(0.0F, 0.0F, 0.0F, 0.0F);
  buffers[0][0].normal = settings.lerpNormals ? *i_vertices[0].normal : Vec4(0.0F, 0.0F, 0.0F, 0.0F);
  buffers[0][0].color = settings.lerpColors ? *i_vertices[0].color : Vec4(1.0F, 1.0F, 1.0F, 1.0F);

  buffers[0][1].position = *i_vertices[1].position;
  buffers[0][1].st = settings.lerpTexCoords ? *i_vertices[1].st : Vec4(0.0F, 0.0F, 0.0F, 0.0F);
  buffers[0][1].normal = settings.lerpNormals ? *i_vertices[1].normal : Vec4(0.0F, 0.0F, 0.0F, 0.0F);
  buffers[0][1].color = settings.lerpColors ? *i_vertices[1].color : Vec4(1.0F, 1.0F, 1.0F, 1.0F);

  buffers[0][2].position = *i_vertices[2].position;
  buffers[0][2].st = settings.lerpTexCoords ? *i_vertices[2].st : Vec4(0.0F, 0.0F, 0.0F, 0.0F);
  buffers[0][2].normal = settings.lerpNormals ? *i_vertices[2].normal : Vec4(0.0F, 0.0F, 0.0F, 0.0F);
  buffers[0][2].color = settings.lerpColors ? *i_vertices[2].color : Vec4(1.0F, 1.0F, 1.0F, 1.0F);

  // Helper lambda for attribute interpolation
  auto lerpV4 = [](const Vec4& a, const Vec4& b, float t) -> Vec4 {
    return Vec4::getByLerp(a, b, t);
  };

  // 2. Clip against all 6 planes
  for (int p = 0; p < 6; p++) {
    if (inCount == 0) break;

    const int outIdx = (inIdx + 1) % 2;
    outCount = 0;
    
    const Plane& plane = frustumPlanes[p];
    
    for (int i = 0; i < inCount; i++) {
        // Safety check
        if (outCount >= 23) break; 

        const PlanesClipVertex& cur = buffers[inIdx][i];
        const PlanesClipVertex& nxt = buffers[inIdx][(i + 1) % inCount];

        float dCur = plane.distanceTo(cur.position);
        float dNxt = plane.distanceTo(nxt.position);

        const bool curInside = dCur >= 0.0F;
        const bool nxtInside = dNxt >= 0.0F;

        if (curInside && nxtInside) {
            // Keep next
            buffers[outIdx][outCount++] = nxt;
        } else if (curInside && !nxtInside) {
            // Leaving -> add intersection
            float t;
            Vec4 pNormal = plane.normal;
            Vec4 curPos = cur.position;
            Vec4 nxtPos = nxt.position;

            Vec4 intersection = intersectPlane(pNormal, curPos, nxtPos, plane.distance, t);
            
            PlanesClipVertex& v = buffers[outIdx][outCount++];
            v.position = intersection;
            if (settings.lerpTexCoords) v.st = lerpV4(cur.st, nxt.st, t);
            if (settings.lerpNormals) v.normal = lerpV4(cur.normal, nxt.normal, t);
            if (settings.lerpColors) v.color = lerpV4(cur.color, nxt.color, t);

        } else if (!curInside && nxtInside) {
            // Entering -> add intersection, then next
            float t;
            Vec4 pNormal = plane.normal;
            Vec4 curPos = cur.position;
            Vec4 nxtPos = nxt.position;

            Vec4 intersection = intersectPlane(pNormal, curPos, nxtPos, plane.distance, t);

            PlanesClipVertex& v = buffers[outIdx][outCount++];
            v.position = intersection;
            if (settings.lerpTexCoords) v.st = lerpV4(cur.st, nxt.st, t);
            if (settings.lerpNormals) v.normal = lerpV4(cur.normal, nxt.normal, t);
            if (settings.lerpColors) v.color = lerpV4(cur.color, nxt.color, t);

            if (outCount < 24) {
                 buffers[outIdx][outCount++] = nxt;
            }
        }
    }

    inCount = outCount;
    inIdx = outIdx;
  }

  // 3. Triangulate
  if (inCount < 3) return 0;

  const PlanesClipVertex& v0 = buffers[inIdx][0];
  int emittedVertices = 0;

  for (int k = 1; k + 1 < inCount; k++) {
    o_vertices.push_back(v0);
    o_vertices.push_back(buffers[inIdx][k]);
    o_vertices.push_back(buffers[inIdx][k + 1]);
    emittedVertices += 3;
  }

  return emittedVertices;
}

// Unused legacy method required by header interface? 
// The header declares: u8 clipAgainstPlane(Triangle& original, PlanesClipVertex* clipped, ...);
// We can keep a stub or remove it from the cpp if we remove it from hpp. 
// For now, I will leave a stub to match the header, but it shouldn't be called.
u8 CustomPlanesClipAlgorithm::clipAgainstPlane(
    Triangle& original, PlanesClipVertex* clipped,
    const EEClipAlgorithmSettings& settings, Plane* plane) {
    return 0; 
}