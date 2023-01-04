#include "managers/block/vertex_block_data.hpp"

VertexBlockData::VertexBlockData() {}

VertexBlockData::~VertexBlockData() {}

const Vec4* VertexBlockData::getVertexData() {
  std::array<Vec4, 8> cornerVetices = {
      Vec4(1.0F, -1.0F, -1.0), Vec4(1.0F, -1.0F, 1.0),
      Vec4(-1.0F, -1.0F, 1.0), Vec4(-1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),  Vec4(1.0F, 1.0F, 1.0),
      Vec4(-1.0F, 1.0F, 1.0),  Vec4(-1.0F, 1.0F, -1.0),
  };

  u32 vertexFacesIndex[VETEX_COUNT] = {// Top
                                       8, 6, 5, 8, 7, 6,

                                       // Bottom
                                       1, 3, 4, 1, 2, 3,

                                       // Left
                                       1, 8, 5, 1, 4, 8,

                                       // Right
                                       6, 3, 2, 6, 7, 3,

                                       // Back
                                       5, 2, 1, 5, 6, 2,

                                       // Front
                                       3, 8, 4, 3, 7, 8,
                                      };

  Vec4* result = new Vec4[VETEX_COUNT];

  for (size_t i = 0; i < VETEX_COUNT; i++) {
    result[i] = cornerVetices[vertexFacesIndex[i] - 1];
  }

  return result;
}

const Vec4* VertexBlockData::getVertexNormalData() {
  Vec4* normals = new Vec4[FACES_COUNT];
  normals[0] = Vec4(0, -1, 0);
  normals[1] = Vec4(0, 1, 0);
  normals[2] = Vec4(-1, 0, 0);
  normals[3] = Vec4(1, 0, 0);
  normals[4] = Vec4(0, 0, -1);
  normals[5] = Vec4(0, 0, 1);
  return normals;
}
