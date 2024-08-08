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
    a.position = Vec4(0, 0, 0);
    a.st = Vec4(0, 0, 0);
    a.normal = Vec4(0, 0, 0);
    a.color = Vec4(0, 0, 0);

    b.position = Vec4(0, 0, 0);
    b.st = Vec4(0, 0, 0);
    b.normal = Vec4(0, 0, 0);
    b.color = Vec4(0, 0, 0);

    c.position = Vec4(0, 0, 0);
    c.st = Vec4(0, 0, 0);
    c.normal = Vec4(0, 0, 0);
    c.color = Vec4(0, 0, 0);
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

  int clip(std::vector<PlanesClipVertex>& o_vertices,
           PlanesClipVertexPtrs* i_vertices,
           const EEClipAlgorithmSettings& settings, Plane* frustumPlanes);

 private:
  Vec4 intersectPlane(Vec4& plane_p, Vec4& plane_n, Vec4& lineStart,
                      Vec4& lineEnd, float plane_d, float& t);

  /** @return clipped size */
  u8 clipAgainstPlane(Triangle& original, PlanesClipVertex* clipped,
                      const EEClipAlgorithmSettings& settings, Plane* plane);
};
