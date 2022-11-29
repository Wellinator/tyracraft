#include "managers/block/vertex_block_data.hpp"

VertexBlockData::VertexBlockData() {}

VertexBlockData::~VertexBlockData() {}

const Vec4* VertexBlockData::getVertexData() {
  std::array<Vec4, 24> tempVertices = {
      Vec4(-1.0F, -1.0F, -1.0F), Vec4(1.0F, -1.0F, -1.0F),
      Vec4(1.0F, -1.0F, 1.0F),   Vec4(-1.0F, -1.0F, 1.0F),
      Vec4(1.0F, -1.0F, 1.0F),   Vec4(1.0F, 1.0F, 1.0F),
      Vec4(-1.0F, 1.0F, 1.0F),   Vec4(-1.0F, -1.0F, 1.0F),
      Vec4(-1.0F, -1.0F, 1.0F),  Vec4(-1.0F, 1.0F, 1.0F),
      Vec4(-1.0F, 1.0F, -1.0F),  Vec4(-1.0F, -1.0F, -1.0F),
      Vec4(1.0F, -1.0F, -1.0F),  Vec4(1.0F, 1.0F, -1.0F),
      Vec4(1.0F, 1.0F, 1.0F),    Vec4(1.0F, -1.0F, 1.0F),
      Vec4(-1.0F, -1.0F, -1.0F), Vec4(-1.0F, 1.0F, -1.0F),
      Vec4(1.0F, 1.0F, -1.0F),   Vec4(1.0F, -1.0F, -1.0F),
      Vec4(1.0F, 1.0F, -1.0F),   Vec4(-1.0F, 1.0F, -1.0F),
      Vec4(-1.0F, 1.0F, 1.0F),   Vec4(1.0F, 1.0F, 1.0F),
  };

  u32 vertexFacesIndex[VETEX_COUNT] = {
      1,  2,  3,  1,  3,  4,  5,  6,  7,  5,  7,  8,  9,  10, 11, 9,  11, 12,
      13, 14, 15, 13, 15, 16, 17, 18, 19, 17, 19, 20, 21, 22, 23, 21, 23, 24};

  Vec4* tempVertFaces = new Vec4[VETEX_COUNT];

  for (size_t i = 0; i < VETEX_COUNT; i++) {
    tempVertFaces[i] = tempVertices[vertexFacesIndex[i] - 1];
  }

  return tempVertFaces;
}
