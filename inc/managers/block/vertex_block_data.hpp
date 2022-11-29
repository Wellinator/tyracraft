#pragma once
#include <tamtypes.h>
#include "tyra"

using Tyra::Vec4;

class VertexBlockData {
 public:
  VertexBlockData();

  ~VertexBlockData();

  static const Vec4* getVertexData();

  static const u8 VETEX_COUNT = 36;
};
