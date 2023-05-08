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
  static const Vec4* getVertexData();

  /**
   * @brief Provides 6 face normals of raw cube
   * @returns new Vec4[6]
   */
  static const Vec4* getVertexNormalData();

  static const u8 VETEX_COUNT = 36;
  static const u8 FACES_COUNT = 6;

  inline const u8* getTopFaceIndexes() { return topFacesIndex; };
  inline const u8* getBottomFaceIndexes() { return bottomFaceIndex; };
  inline const u8* getLeftFaceIndexes() { return leftFaceIndex; };
  inline const u8* getRightFaceIndexes() { return rightFaceIndex; };
  inline const u8* getBackFaceIndexes() { return backFaceIndex; };
  inline const u8* getFrontFaceIndexes() { return frontFaceIndex; };

 private:
  u8 topFacesIndex[FACES_COUNT] = {0, 1, 2, 3, 4, 5};
  u8 bottomFaceIndex[FACES_COUNT] = {6, 7, 8, 9, 10, 11};
  u8 leftFaceIndex[FACES_COUNT] = {12, 13, 14, 15, 16, 17};
  u8 rightFaceIndex[FACES_COUNT] = {18, 19, 20, 21, 22, 23};
  u8 backFaceIndex[FACES_COUNT] = {24, 25, 26, 27, 28, 29};
  u8 frontFaceIndex[FACES_COUNT] = {30, 31, 32, 33, 34, 35};
};
