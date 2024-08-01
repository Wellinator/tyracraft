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
#include <queue>

CustomPlanesClipAlgorithm::CustomPlanesClipAlgorithm() {}

CustomPlanesClipAlgorithm::~CustomPlanesClipAlgorithm() {}

Vec4 CustomPlanesClipAlgorithm::intersectPlane(Vec4& plane_p, Vec4& plane_n,
                                               Vec4& lineStart, Vec4& lineEnd,
                                               float plane_d, float& t) {
  Vec4 n = plane_n;
  Vec4 p = lineStart;
  Vec4 d = (lineEnd - lineStart).getNormalized();

  double denominator = d.dot3(n);

  t = -((n.dot3(p)) + plane_d) / denominator;
  return p + (d * t);
}

u8 CustomPlanesClipAlgorithm::clip(std::vector<PlanesClipVertex>& o_vertices,
                                   PlanesClipVertexPtrs* i_vertices,
                                   const EEClipAlgorithmSettings& settings,
                                   Plane* frustumPlanes) {
  std::queue<Triangle> trianglesToClip = {};

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

  trianglesToClip.push(initialInput);
  int nNewTriangles = 1;

  for (u8 i = 0; i < 6; i++) {
    u8 clipped = 0;

    while (nNewTriangles > 0) {
      PlanesClipVertex tempVertices[9];
      Triangle input = trianglesToClip.front();
      trianglesToClip.pop();
      nNewTriangles--;

      clipped =
          clipAgainstPlane(input, tempVertices, settings, &frustumPlanes[i]);

      for (size_t j = 0; j < clipped; j++) {
        Triangle newtri(tempVertices[j * 3], tempVertices[j * 3 + 1],
                        tempVertices[j * 3 + 2]);

        trianglesToClip.push(newtri);
      }
    }

    nNewTriangles = trianglesToClip.size();
  }

  u8 result = trianglesToClip.size();
  o_vertices.resize(result * 3, {Vec4()});

  int i = 0;
  while (!trianglesToClip.empty()) {
    Triangle t = trianglesToClip.front();
    trianglesToClip.pop();

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
  int nInsidePointCount = 0;
  Vec4 outside_points[3];
  int nOutsidePointCount = 0;

  Vec4 inside_tex[3];
  int nInsideTexCount = 0;
  Vec4 outside_tex[3];
  int nOutsideTexCount = 0;

  auto a = original.a;
  auto b = original.b;
  auto c = original.c;

  // Get signed distance of each point in triangle to plane
  float d0 = plane->distanceTo(a.position);  // dist(a.position);
  float d1 = plane->distanceTo(b.position);  // dist(b.position);
  float d2 = plane->distanceTo(c.position);  // dist(c.position);

  if (d0 >= 0) {
    inside_points[nInsidePointCount++] = a.position;
    inside_tex[nInsideTexCount++] = a.st;
  } else {
    outside_points[nOutsidePointCount++] = a.position;
    outside_tex[nOutsideTexCount++] = a.st;
  }
  if (d1 >= 0) {
    inside_points[nInsidePointCount++] = b.position;
    inside_tex[nInsideTexCount++] = b.st;
  } else {
    outside_points[nOutsidePointCount++] = b.position;
    outside_tex[nOutsideTexCount++] = b.st;
  }
  if (d2 >= 0) {
    inside_points[nInsidePointCount++] = c.position;
    inside_tex[nInsideTexCount++] = c.st;
  } else {
    outside_points[nOutsidePointCount++] = c.position;
    outside_tex[nOutsideTexCount++] = c.st;
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
    // Copy appearance info to new triangle
    // The inside point is valid, so keep that...
    clipped[0].position = inside_points[0];
    clipped[0].st = inside_tex[0];
    clipped[0].color = a.color;

    float p;
    clipped[1].position =
        intersectPlane(plane_p, plane->normal, inside_points[0],
                       outside_points[0], plane->distance, p);
    // TYRA_LOG("f: ", p);

    if (settings.lerpNormals)
      clipped[1].normal = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);

    if (settings.lerpTexCoords) {
      // clipped[1].st = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);
      // out_tri1.t[1].u = t * (outside_tex[0]->u - inside_tex[0]->u) +
      // inside_tex[0]->u;
      clipped[1].st = (outside_tex[0] - inside_tex[0]) * p + inside_tex[0];
    }

    // if (settings.lerpColors){
    //   clipped[1].color = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);
    // }

    clipped[2].position =
        intersectPlane(plane_p, plane->normal, inside_points[0],
                       outside_points[1], plane->distance, p);
    if (settings.lerpNormals)
      clipped[2].normal = Vec4::getByLerp(outside_tex[1], inside_tex[0], p);

    if (settings.lerpTexCoords) {
      // clipped[2].st = Vec4::getByLerp(outside_tex[1], inside_tex[0], p);
      // out_tri1.t[2].u = t * (outside_tex[1]->u - inside_tex[0]->u) +
      // inside_tex[0]->u;
      clipped[2].st = (outside_tex[1] - inside_tex[0]) * p + inside_tex[0];
    }

    // if (settings.lerpColors)
    //   clipped[2].color = Vec4::getByLerp(outside_tex[1], inside_tex[0], p);

    return 1;  // Return the newly formed single triangle
  }

  if (nInsidePointCount == 2 && nOutsidePointCount == 1) {
    // Copy appearance info to new triangle
    clipped[0].color = a.color;
    clipped[1].color = b.color;
    clipped[2].color = c.color;

    // Copy appearance info to new triangle
    clipped[3].color = a.color;
    clipped[4].color = b.color;
    clipped[5].color = c.color;

    // The first triangle consists of the two inside points and a new
    // point determined by the location where one side of the triangle
    // intersects with the plane
    clipped[0].position = inside_points[0];
    clipped[1].position = inside_points[1];
    clipped[0].st = inside_tex[0];
    clipped[1].st = inside_tex[1];

    float p;
    clipped[2].position =
        intersectPlane(plane_p, plane->normal, inside_points[0],
                       outside_points[0], plane->distance, p);

    if (settings.lerpNormals)
      clipped[2].normal = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);

    if (settings.lerpTexCoords) {
      // clipped[2].st = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);
      // out_tri1.t[2].u = t * (outside_tex[0]->u - inside_tex[0]->u) +
      // inside_tex[0]->u;
      clipped[2].st = (outside_tex[0] - inside_tex[0]) * p + inside_tex[0];
    }

    // if (settings.lerpColors)
    //   clipped[2].color = Vec4::getByLerp(outside_tex[0], inside_tex[0], p);

    // The second triangle is composed of one of he inside points, a
    // new point determined by the intersection of the other side of the
    // triangle and the plane, and the newly created point above
    clipped[3].position = inside_points[1];
    clipped[3].st = inside_tex[1];
    clipped[4].position = clipped[2].position;
    clipped[4].st = clipped[2].st;

    clipped[5].position =
        intersectPlane(plane_p, plane->normal, inside_points[1],
                       outside_points[0], plane->distance, p);

    if (settings.lerpNormals)
      clipped[5].normal = Vec4::getByLerp(outside_tex[0], inside_tex[1], p);

    if (settings.lerpTexCoords) {
      // clipped[2].st = Vec4::getByLerp(outside_tex[0], inside_tex[1], p);
      // out_tri2.t[2].u = t * (outside_tex[0]->u - inside_tex[1]->u) +
      // inside_tex[1]->u;
      clipped[5].st = (outside_tex[0] - inside_tex[1]) * p + inside_tex[1];
    }

    // if (settings.lerpColors)
    //   clipped[5].color = Vec4::getByLerp(outside_tex[0], inside_tex[1], p);

    return 2;  // Return two newly formed triangles which form a quad
  }

  return 0;
}