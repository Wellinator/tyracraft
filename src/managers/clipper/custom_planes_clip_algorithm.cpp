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

CustomPlanesClipAlgorithm::CustomPlanesClipAlgorithm() {}

CustomPlanesClipAlgorithm::~CustomPlanesClipAlgorithm() {}

Vec4 CustomPlanesClipAlgorithm::intersectPlane(Vec4& plane_p, Vec4& plane_n,
                                               Vec4& lineStart, Vec4& lineEnd,
                                               float plane_d, float& t) {
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
  std::list<Triangle> trianglesToClip;
  PlanesClipVertex tempVertices[6] = {};

  Triangle initialInput;

  initialInput.a.position = *i_vertices[0].position;
  if (settings.lerpColors) initialInput.a.color = *i_vertices[0].color;
  if (settings.lerpNormals) initialInput.a.normal = *i_vertices[0].normal;
  if (settings.lerpTexCoords) initialInput.a.st = *i_vertices[0].st;

  initialInput.b.position = *i_vertices[1].position;
  if (settings.lerpColors) initialInput.b.color = *i_vertices[1].color;
  if (settings.lerpNormals) initialInput.b.normal = *i_vertices[1].normal;
  if (settings.lerpTexCoords) initialInput.b.st = *i_vertices[1].st;

  initialInput.c.position = *i_vertices[2].position;
  if (settings.lerpColors) initialInput.c.color = *i_vertices[2].color;
  if (settings.lerpNormals) initialInput.c.normal = *i_vertices[2].normal;
  if (settings.lerpTexCoords) initialInput.c.st = *i_vertices[2].st;

  trianglesToClip.push_back(initialInput);
  int nNewTriangles = 1;

  for (u8 i = 0; i < 6; i++) {
    u8 clipped = 0;

    while (nNewTriangles > 0) {
      Triangle input = trianglesToClip.front();
      trianglesToClip.pop_front();
      nNewTriangles--;

      clipped =
          clipAgainstPlane(input, tempVertices, settings, &frustumPlanes[i]);

      if (clipped == 0) {
        continue;
      } else if (clipped == 1) {
        trianglesToClip.emplace_back(tempVertices[0], tempVertices[1],
                                     tempVertices[2]);
      } else if (clipped == 2) {
        trianglesToClip.emplace_back(tempVertices[0], tempVertices[1],
                                     tempVertices[2]);

        trianglesToClip.emplace_back(tempVertices[3], tempVertices[4],
                                     tempVertices[5]);
      }
    }

    nNewTriangles = trianglesToClip.size();
  }

  o_vertices.resize(nNewTriangles * 3, {Vec4(0, 0, 0), Vec4(0, 0, 0),
                                        Vec4(0, 0, 0), Vec4(0, 0, 0)});

  int i = 0;
  for (auto& t : trianglesToClip) {
    o_vertices[i].position = t.a.position;
    if (settings.lerpColors) o_vertices[i].color = t.a.color;
    if (settings.lerpNormals) o_vertices[i].normal = t.a.normal;
    if (settings.lerpTexCoords) o_vertices[i].st = t.a.st;

    i++;
    o_vertices[i].position = t.b.position;
    if (settings.lerpColors) o_vertices[i].color = t.b.color;
    if (settings.lerpNormals) o_vertices[i].normal = t.b.normal;
    if (settings.lerpTexCoords) o_vertices[i].st = t.b.st;

    i++;
    o_vertices[i].position = t.c.position;
    if (settings.lerpColors) o_vertices[i].color = t.c.color;
    if (settings.lerpNormals) o_vertices[i].normal = t.c.normal;
    if (settings.lerpTexCoords) o_vertices[i].st = t.c.st;

    i++;
  }

  // If was clippled by any plane, return the clipped size else, return zero;
  return o_vertices.size();
}

u8 CustomPlanesClipAlgorithm::clipAgainstPlane(
    Triangle& original, PlanesClipVertex* clipped,
    const EEClipAlgorithmSettings& settings, Plane* plane) {
  Vec4 plane_p = plane->normal * plane->distance;

  Vec4 inside_points[3];
  u8 nInsidePointCount = 0;
  Vec4 outside_points[3];
  u8 nOutsidePointCount = 0;

  Vec4 inside_tex[3];
  u8 nInsideTexCount = 0;
  Vec4 outside_tex[3];
  u8 nOutsideTexCount = 0;

  Vec4 inside_col[3];
  u8 nInsideColCount = 0;
  Vec4 outside_col[3];
  u8 nOutsideColCount = 0;

  PlanesClipVertex& a = original.a;
  PlanesClipVertex& b = original.b;
  PlanesClipVertex& c = original.c;

  // Get signed distance of each point in triangle to plane
  float d0 = plane->distanceTo(a.position);
  float d1 = plane->distanceTo(b.position);
  float d2 = plane->distanceTo(c.position);

  if (d0 >= 0) {
    inside_points[nInsidePointCount++] = a.position;
    inside_tex[nInsideTexCount++] = a.st;
    inside_col[nInsideColCount++] = a.color;
  } else {
    outside_points[nOutsidePointCount++] = a.position;
    outside_tex[nOutsideTexCount++] = a.st;
    outside_col[nOutsideColCount++] = a.color;
  }
  if (d1 >= 0) {
    inside_points[nInsidePointCount++] = b.position;
    inside_tex[nInsideTexCount++] = b.st;
    inside_col[nInsideColCount++] = b.color;
  } else {
    outside_points[nOutsidePointCount++] = b.position;
    outside_tex[nOutsideTexCount++] = b.st;
    outside_col[nOutsideColCount++] = b.color;
  }
  if (d2 >= 0) {
    inside_points[nInsidePointCount++] = c.position;
    inside_tex[nInsideTexCount++] = c.st;
    inside_col[nInsideColCount++] = c.color;
  } else {
    outside_points[nOutsidePointCount++] = c.position;
    outside_tex[nOutsideTexCount++] = c.st;
    outside_col[nOutsideColCount++] = c.color;
  }

  if (nInsidePointCount == 0) {
    // All points lie on the outside of plane, so clip whole triangle
    // It ceases to exist

    // No returned triangles are valid;
    return 0;
  }

  if (nInsidePointCount == 3) {
    // All points lie on the inside of plane, so do nothing
    // and allow the triangle to simply pass through

    // Just the one returned original triangle is valid
    clipped[0].position = a.position;
    clipped[1].position = b.position;
    clipped[2].position = c.position;

    if (settings.lerpColors) {
      clipped[0].color = a.color;
      clipped[1].color = b.color;
      clipped[2].color = c.color;
    }

    if (settings.lerpTexCoords) {
      clipped[0].st = a.st;
      clipped[1].st = b.st;
      clipped[2].st = c.st;
    }

    if (settings.lerpNormals) {
      clipped[0].normal = a.normal;
      clipped[1].normal = b.normal;
      clipped[2].normal = c.normal;
    }

    return 1;
  }

  if (nInsidePointCount == 1 && nOutsidePointCount == 2) {
    // Distance from inside to plane intersection
    float p;

    // Copy appearance info to new triangle
    // The inside point is valid, so keep that...
    clipped[0].position = inside_points[0];
    clipped[0].st = inside_tex[0];
    clipped[0].color = inside_col[0];

    // but the two new points are at the locations where the
    // original sides of the triangle (lines) intersect with the plane
    clipped[1].position =
        intersectPlane(plane_p, plane->normal, inside_points[0],
                       outside_points[0], plane->distance, p);

    if (settings.lerpNormals) {
      clipped[1].normal = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);
    }

    if (settings.lerpTexCoords) {
      clipped[1].st = Vec4::getByLerp(inside_tex[0], outside_tex[0], p);
    }

    if (settings.lerpColors) {
      clipped[1].color = Vec4::getByLerp(inside_col[0], outside_col[0], p);
    }

    clipped[2].position =
        intersectPlane(plane_p, plane->normal, inside_points[0],
                       outside_points[1], plane->distance, p);

    if (settings.lerpNormals) {
      clipped[2].normal = Vec4::getByLerp(outside_tex[1], inside_tex[0], p);
    }

    if (settings.lerpTexCoords) {
      clipped[2].st = Vec4::getByLerp(inside_tex[0], outside_tex[1], p);
    }

    if (settings.lerpColors) {
      clipped[2].color = Vec4::getByLerp(inside_col[0], outside_col[1], p);
    }

    return 1;  // Return the newly formed single triangle
  }

  if (nInsidePointCount == 2 && nOutsidePointCount == 1) {
    // Distance from inside to plane intersection
    float p;

    // The first triangle consists of the two inside points and a new
    // point determined by the location where one side of the triangle
    // intersects with the plane
    clipped[0].position = inside_points[0];
    clipped[0].st = inside_tex[0];
    clipped[0].color = inside_col[0];

    clipped[1].position = inside_points[1];
    clipped[1].st = inside_tex[1];
    clipped[1].color = inside_col[1];

    clipped[2].position =
        intersectPlane(plane_p, plane->normal, inside_points[0],
                       outside_points[0], plane->distance, p);

    if (settings.lerpNormals) {
      clipped[2].normal = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);
    }

    if (settings.lerpTexCoords) {
      clipped[2].st = Vec4::getByLerp(inside_tex[0], outside_tex[0], p);
    }

    if (settings.lerpColors) {
      clipped[2].color = Vec4::getByLerp(inside_col[0], outside_col[0], p);
    }

    // The second triangle is composed of one of he inside points, a
    // new point determined by the intersection of the other side of the
    // triangle and the plane, and the newly created point above
    clipped[3].position = inside_points[1];
    clipped[3].st = inside_tex[1];
    clipped[3].color = inside_col[1];

    clipped[4].position = clipped[2].position;
    clipped[4].st = clipped[2].st;
    clipped[4].color = clipped[2].color;

    clipped[5].position =
        intersectPlane(plane_p, plane->normal, inside_points[1],
                       outside_points[0], plane->distance, p);

    if (settings.lerpNormals) {
      clipped[5].normal = Vec4::getByLerp(inside_tex[1], outside_tex[0], p);
    }

    if (settings.lerpTexCoords) {
      clipped[5].st = Vec4::getByLerp(inside_tex[1], outside_tex[0], p);
    }

    if (settings.lerpColors) {
      clipped[5].color = Vec4::getByLerp(inside_col[1], outside_col[0], p);
    }

    return 2;  // Return two newly formed triangles which form a quad
  }

  TYRA_TRAP("Invalid clipping values!");
  return 0;
}