/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2024, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Wellington Carvalho (Wellinator) <wellcoj@gmail.com>
*/

#pragma once

#include <tyra>
#include <vector>

using Tyra::EEClipAlgorithmSettings;
using Tyra::Plane;
using Tyra::PlanesClipVertex;
using Tyra::PlanesClipVertexPtrs;
using Tyra::RendererSettings;
using Tyra::Vec4;

struct Triangle {
  PlanesClipVertex a;
  PlanesClipVertex b;
  PlanesClipVertex c;

  Triangle() {
    a.position = Vec4();
    a.st = Vec4();
    a.normal = Vec4();
    a.color = Vec4();

    b.position = Vec4();
    b.st = Vec4();
    b.normal = Vec4();
    b.color = Vec4();

    c.position = Vec4();
    c.st = Vec4();
    c.normal = Vec4();
    c.color = Vec4();
  }

  Triangle(PlanesClipVertex _a, PlanesClipVertex _b, PlanesClipVertex _c) {
    a = _a;
    b = _b;
    c = _c;
  }
};

class CustomPlanesClipAlgorithm {
 public:
  CustomPlanesClipAlgorithm();
  ~CustomPlanesClipAlgorithm();

  u8 clip(PlanesClipVertex* o_vertices, PlanesClipVertexPtrs* i_vertices,
          const EEClipAlgorithmSettings& settings, Plane* frustumPlanes);

 private:
  PlanesClipVertex* tempVertices;

  Vec4 intersectPlane(Vec4& plane_p, Vec4& plane_n, Vec4& lineStart,
                      Vec4& lineEnd, float plane_d, float& t);

  /** @return clipped size */
  u8 clipAgainstPlane(Triangle& original, PlanesClipVertex* clipped,
                      const EEClipAlgorithmSettings& settings, Plane* plane);
};
