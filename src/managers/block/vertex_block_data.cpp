#include "managers/block/vertex_block_data.hpp"

VertexBlockData::VertexBlockData() {}

VertexBlockData::~VertexBlockData() {}

const Vec4* VertexBlockData::getVertexData() {
  Vec4 cornerVetices[8] = {
      Vec4(1.0F, -1.0F, -1.0), Vec4(1.0F, -1.0F, 1.0),
      Vec4(-1.0F, -1.0F, 1.0), Vec4(-1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),  Vec4(1.0F, 1.0F, 1.0),
      Vec4(-1.0F, 1.0F, 1.0),  Vec4(-1.0F, 1.0F, -1.0),
  };

  u32 vertexFacesIndex[VETEX_COUNT] = {// Top OK
                                       5, 7, 8, 5, 6, 7,

                                       // Bottom
                                       2, 4, 3, 2, 1, 4,

                                       // Left
                                       2, 5, 1, 2, 6, 5,

                                       // Right OK
                                       4, 7, 3, 4, 8, 7,

                                       // Back
                                       3, 6, 2, 3, 7, 6,

                                       // Front
                                       1, 8, 4, 1, 5, 8};

  Vec4* result = new Vec4[VETEX_COUNT];

  for (size_t i = 0; i < VETEX_COUNT; i++) {
    result[i] = cornerVetices[vertexFacesIndex[i] - 1];
  }

  return result;
}

const Vec4* VertexBlockData::getCrossedVertexData() {
  Vec4 cornerVetices[8] = {
      Vec4(1.0F, -1.0F, -1.0), Vec4(1.0F, -1.0F, 1.0),
      Vec4(-1.0F, -1.0F, 1.0), Vec4(-1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),  Vec4(1.0F, 1.0F, 1.0),
      Vec4(-1.0F, 1.0F, 1.0),  Vec4(-1.0F, 1.0F, -1.0),
  };

  u8 vertexFacesIndex[CROSSED_VETEX_COUNT] = {// Face 1
                                              1, 7, 3, 1, 5, 7,

                                              // Face 2
                                              4, 6, 2, 4, 8, 6};

  Vec4* result = new Vec4[CROSSED_VETEX_COUNT];

  for (size_t i = 0; i < CROSSED_VETEX_COUNT; i++) {
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
