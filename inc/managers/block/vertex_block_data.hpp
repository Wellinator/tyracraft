#pragma once
#include <tamtypes.h>
#include "tyra"

using Tyra::Vec4;

class VertexBlockData {
 public:
  VertexBlockData();

  ~VertexBlockData();

  /**
   * @brief Provides 36 vertices of raw cube
   * @returns new Vec4[36]
   */
  const Vec4* getVertexData();

  /**
   * @brief Provides 6 face normals of raw cube
   * @returns new Vec4[6]
   */
  const Vec4* getVertexNormalData();

  const u8 VETEX_COUNT = 36;
  const u8 FACES_COUNT = 6;
};
