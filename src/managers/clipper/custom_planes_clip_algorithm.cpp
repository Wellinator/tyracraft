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
  // Vec4 dir = (lineEnd - lineStart).getNormalized();
  // t = -((plane_n.dot3(lineStart)) + plane_d) / dir.dot3(plane_n);
  // return lineStart + (dir * t);

  float d1 = plane_d + plane_n.innerProduct(lineStart);
  float d2 = plane_d + plane_n.innerProduct(lineEnd);

  t = d1 / (d1 - d2);
  Vec4 result = lineStart + ((lineEnd - lineStart) * t);

  // Fix W values
  result.w = 1.0F;

  return result;
}

int CustomPlanesClipAlgorithm::clip(std::vector<PlanesClipVertex>& o_vertices,
                                    PlanesClipVertexPtrs* i_vertices,
                                    const EEClipAlgorithmSettings& settings,
                                    Plane* frustumPlanes) {
  // Use double-buffered vectors to avoid list churn and reallocation
  static std::vector<Triangle> cur;
  static std::vector<Triangle> next;
  
  cur.clear();
  next.clear();
  cur.reserve(32);
  next.reserve(32);

  PlanesClipVertex tempVertices[6] = {};

  // Construir triângulo inicial de forma mais eficiente
  Triangle initialInput;
  
  // Vértice A - sempre copiar posição
  initialInput.a.position = *i_vertices[0].position;
  initialInput.a.color = settings.lerpColors ? *i_vertices[0].color : Vec4(1.0F, 1.0F, 1.0F, 1.0F);
  initialInput.a.normal = settings.lerpNormals ? *i_vertices[0].normal : Vec4(0, 0, 0);
  initialInput.a.st = settings.lerpTexCoords ? *i_vertices[0].st : Vec4(0, 0, 0);

  // Vértice B
  initialInput.b.position = *i_vertices[1].position;
  initialInput.b.color = settings.lerpColors ? *i_vertices[1].color : Vec4(1.0F, 1.0F, 1.0F, 1.0F);
  initialInput.b.normal = settings.lerpNormals ? *i_vertices[1].normal : Vec4(0, 0, 0);
  initialInput.b.st = settings.lerpTexCoords ? *i_vertices[1].st : Vec4(0, 0, 0);

  // Vértice C
  initialInput.c.position = *i_vertices[2].position;
  initialInput.c.color = settings.lerpColors ? *i_vertices[2].color : Vec4(1.0F, 1.0F, 1.0F, 1.0F);
  initialInput.c.normal = settings.lerpNormals ? *i_vertices[2].normal : Vec4(0, 0, 0);
  initialInput.c.st = settings.lerpTexCoords ? *i_vertices[2].st : Vec4(0, 0, 0);

  cur.emplace_back(initialInput);

  // Check against left, right, top and bottom planes
  for (u8 p = 0; p < 4; p++) {
    next.clear();
    for (size_t ti = 0; ti < cur.size(); ++ti) {
      Triangle& input = cur[ti];
      const u8 clipped =
          clipAgainstPlane(input, tempVertices, settings, &frustumPlanes[p]);
      if (clipped == 1) {
        next.emplace_back(tempVertices[0], tempVertices[1], tempVertices[2]);
      } else if (clipped == 2) {
        next.emplace_back(tempVertices[0], tempVertices[1], tempVertices[2]);
        next.emplace_back(tempVertices[3], tempVertices[4], tempVertices[5]);
      }
      // clipped == 0 -> triangle fully discarded
    }
    cur.swap(next);
    if (cur.empty()) break;  // Early out if everything got clipped away
  }

  const size_t outCount = cur.size() * 3;
  o_vertices.clear();
  o_vertices.reserve(outCount);

  // Otimização: tirar branches do loop usando constantes pré-calculadas
  static const Vec4 defaultColor(1.0F, 1.0F, 1.0F, 1.0F);
  static const Vec4 defaultNormal(0, 0, 0);
  static const Vec4 defaultST(0, 0, 0);

  for (const auto& t : cur) {
    // Vértice A - usar emplace_back diretamente
    o_vertices.emplace_back(PlanesClipVertex{
        t.a.position,
        settings.lerpNormals ? t.a.normal : defaultNormal,
        settings.lerpTexCoords ? t.a.st : defaultST,
        settings.lerpColors ? t.a.color : defaultColor});

    // Vértice B
    o_vertices.emplace_back(PlanesClipVertex{
        t.b.position,
        settings.lerpNormals ? t.b.normal : defaultNormal,
        settings.lerpTexCoords ? t.b.st : defaultST,
        settings.lerpColors ? t.b.color : defaultColor});

    // Vértice C
    o_vertices.emplace_back(PlanesClipVertex{
        t.c.position,
        settings.lerpNormals ? t.c.normal : defaultNormal,
        settings.lerpTexCoords ? t.c.st : defaultST,
        settings.lerpColors ? t.c.color : defaultColor});
  }

  return static_cast<int>(o_vertices.size());
}

u8 CustomPlanesClipAlgorithm::clipAgainstPlane(
    Triangle& original, PlanesClipVertex* clipped,
    const EEClipAlgorithmSettings& settings, Plane* plane) {
  // Usar estrutura compacta para reduzir alocações
  PlanesClipVertex inside_verts[3];
  u8 nInsidePointCount = 0;
  PlanesClipVertex outside_verts[3];
  u8 nOutsidePointCount = 0;

  PlanesClipVertex& a = original.a;
  PlanesClipVertex& b = original.b;
  PlanesClipVertex& c = original.c;

  // Get signed distance of each point in triangle to plane
  float d0 = plane->distanceTo(a.position);
  float d1 = plane->distanceTo(b.position);
  float d2 = plane->distanceTo(c.position);

  // Classificar vértices usando estrutura compacta (menos cópias)
  if (d0 >= 0) {
    inside_verts[nInsidePointCount++] = a;
  } else {
    outside_verts[nOutsidePointCount++] = a;
  }
  
  if (d1 >= 0) {
    inside_verts[nInsidePointCount++] = b;
  } else {
    outside_verts[nOutsidePointCount++] = b;
  }
  
  if (d2 >= 0) {
    inside_verts[nInsidePointCount++] = c;
  } else {
    outside_verts[nOutsidePointCount++] = c;
  }

  // Early returns para casos triviais
  if (nInsidePointCount == 0) {
    // All points lie on the outside of plane, so clip whole triangle
    return 0;
  }

  if (nInsidePointCount == 3) {
    // All points lie on the inside of plane - copiar diretamente
    clipped[0] = a;
    clipped[1] = b;
    clipped[2] = c;
    return 1;
  }

  if (nInsidePointCount == 1 && nOutsidePointCount == 2) {
    // Distance from inside to plane intersection
    float p;

    // The inside point is valid, so keep that
    clipped[0] = inside_verts[0];

    // Two new points at intersection locations
    clipped[1].position = intersectPlane(plane->normal, inside_verts[0].position,
                                         outside_verts[0].position, plane->distance, p);
    if (settings.lerpNormals) {
      clipped[1].normal = Vec4::getByLerp(inside_verts[0].normal, outside_verts[0].normal, p);
    }
    if (settings.lerpTexCoords) {
      clipped[1].st = Vec4::getByLerp(inside_verts[0].st, outside_verts[0].st, p);
    }
    if (settings.lerpColors) {
      clipped[1].color = Vec4::getByLerp(inside_verts[0].color, outside_verts[0].color, p);
    }

    clipped[2].position = intersectPlane(plane->normal, inside_verts[0].position,
                                         outside_verts[1].position, plane->distance, p);
    if (settings.lerpNormals) {
      clipped[2].normal = Vec4::getByLerp(inside_verts[0].normal, outside_verts[1].normal, p);
    }
    if (settings.lerpTexCoords) {
      clipped[2].st = Vec4::getByLerp(inside_verts[0].st, outside_verts[1].st, p);
    }
    if (settings.lerpColors) {
      clipped[2].color = Vec4::getByLerp(inside_verts[0].color, outside_verts[1].color, p);
    }

    return 1;  // Return the newly formed single triangle
  }

  if (nInsidePointCount == 2 && nOutsidePointCount == 1) {
    // Distance from inside to plane intersection
    float p;

    // First triangle: two inside points + one intersection
    clipped[0] = inside_verts[0];
    clipped[1] = inside_verts[1];

    clipped[2].position = intersectPlane(plane->normal, inside_verts[0].position,
                                         outside_verts[0].position, plane->distance, p);
    if (settings.lerpNormals) {
      clipped[2].normal = Vec4::getByLerp(inside_verts[0].normal, outside_verts[0].normal, p);
    }
    if (settings.lerpTexCoords) {
      clipped[2].st = Vec4::getByLerp(inside_verts[0].st, outside_verts[0].st, p);
    }
    if (settings.lerpColors) {
      clipped[2].color = Vec4::getByLerp(inside_verts[0].color, outside_verts[0].color, p);
    }

    // Second triangle: one inside point + two intersections (shared vertex)
    clipped[3] = inside_verts[1];
    clipped[4] = clipped[2];  // Shared vertex from first triangle

    clipped[5].position = intersectPlane(plane->normal, inside_verts[1].position,
                                         outside_verts[0].position, plane->distance, p);
    if (settings.lerpNormals) {
      clipped[5].normal = Vec4::getByLerp(inside_verts[1].normal, outside_verts[0].normal, p);
    }
    if (settings.lerpTexCoords) {
      clipped[5].st = Vec4::getByLerp(inside_verts[1].st, outside_verts[0].st, p);
    }
    if (settings.lerpColors) {
      clipped[5].color = Vec4::getByLerp(inside_verts[1].color, outside_verts[0].color, p);
    }

    return 2;  // Return two newly formed triangles which form a quad
  }

  TYRA_TRAP("Invalid clipping values!");
  return 0;
}