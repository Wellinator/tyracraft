#include "managers/block/vertex_block_data.hpp"
#include "entities/level.hpp"
#include "constants.hpp"

VertexBlockData::VertexBlockData() {}

VertexBlockData::~VertexBlockData() {}

const Vec4* VertexBlockData::getVertexData() {
  Vec4 cornerVetices[8] = {
      Vec4(1.0F, -1.0F, -1.0F), Vec4(1.0F, -1.0F, 1.0F),
      Vec4(-1.0F, -1.0F, 1.0F), Vec4(-1.0F, -1.0F, -1.0F),
      Vec4(1.0F, 1.0F, -1.0F),  Vec4(1.0F, 1.0F, 1.0F),
      Vec4(-1.0F, 1.0F, 1.0F),  Vec4(-1.0F, 1.0F, -1.0F),
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

const Vec4* VertexBlockData::getBottomSlabVertexData() {
  Vec4 cornerVetices[8] = {
      Vec4(1.0F, -1.0F, -1.0F), Vec4(1.0F, -1.0F, 1.0F),
      Vec4(-1.0F, -1.0F, 1.0F), Vec4(-1.0F, -1.0F, -1.0F),
      Vec4(1.0F, 0.0F, -1.0F),  Vec4(1.0F, 0.0F, 1.0F),
      Vec4(-1.0F, 0.0F, 1.0F),  Vec4(-1.0F, 0.0F, -1.0F),
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

const Vec4* VertexBlockData::getTopSlabVertexData() {
  Vec4 cornerVetices[8] = {
      Vec4(1.0F, 0.0F, -1.0F), Vec4(1.0F, 0.0F, 1.0F),
      Vec4(-1.0F, 0.0F, 1.0F), Vec4(-1.0F, 0.0F, -1.0F),
      Vec4(1.0F, 1.0F, -1.0F),  Vec4(1.0F, 1.0F, 1.0F),
      Vec4(-1.0F, 1.0F, 1.0F),  Vec4(-1.0F, 1.0F, -1.0F),
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

const Vec4* VertexBlockData::getTorchVertexData() {
  // List of geometric vertices
  Vec4 vertices[8] = {
      Vec4(0.125000F, -1.000000F, 0.125000F),
      Vec4(0.125000F, -1.000000F, -0.125000F),
      Vec4(0.125000F, 0.250000F, -0.125000F),
      Vec4(0.125000F, 0.250000F, 0.125000F),
      Vec4(-0.125000F, 0.250000F, -0.125000F),
      Vec4(-0.125000F, -1.000000F, -0.125000F),
      Vec4(-0.125000F, -1.000000F, 0.125000F),
      Vec4(-0.125000F, 0.250000F, 0.125000F),
  };

  u32 vertexIndices[VETEX_COUNT] = {
      2, 4, 1, 6, 8, 5, 2, 7, 6, 3, 8, 4, 2, 5, 3, 1, 8, 7,
      2, 3, 4, 6, 7, 8, 2, 1, 7, 3, 5, 8, 2, 6, 5, 1, 4, 8,
  };

  Vec4* result = new Vec4[VETEX_COUNT];

  for (size_t i = 0; i < VETEX_COUNT; i++) {
    result[i] = vertices[vertexIndices[i] - 1];
  }

  return result;
}

const Vec4* VertexBlockData::getTorchUVData() {
  // List of uv mapping
  Vec4 uvData[TORCH_UV_COUNTER] = {
      Vec4(0.714844F, 1.000000F, 1.0F, 0.0F),
      Vec4(0.722656F, 0.960938F, 1.0F, 0.0F),
      Vec4(0.722656F, 1.000000F, 1.0F, 0.0F),
      Vec4(0.714844F, 0.960938F, 1.0F, 0.0F),
      Vec4(0.722656F, 0.992188F, 1.0F, 0.0F),
      Vec4(0.714844F, 0.992188F, 1.0F, 0.0F),
      Vec4(0.722656F, 0.968750F, 1.0F, 0.0F),
      Vec4(0.714844F, 0.968750F, 1.0F, 0.0F),
  };

  u32 vertexIndices[VETEX_COUNT] = {
      1, 2, 3, 3, 4, 2, 5, 1, 6, 7, 4, 2, 3, 4, 2, 1, 2, 3,
      1, 4, 2, 3, 1, 4, 5, 3, 1, 7, 8, 4, 3, 1, 4, 1, 4, 2,

  };

  Vec4* result = new Vec4[VETEX_COUNT];

  for (size_t i = 0; i < VETEX_COUNT; i++) {
    result[i] = uvData[vertexIndices[i] - 1];
  }

  return result;
}

BBox* VertexBlockData::getTorchRawBBox() {
  const auto vertexData = VertexBlockData::getTorchVertexData();
  auto result = new BBox(vertexData, VETEX_COUNT);

  delete vertexData;
  return result;
}

BBox* VertexBlockData::getCuboidRawBBox() {
  const auto vertexData = VertexBlockData::getVertexData();
  auto result = new BBox(vertexData, VETEX_COUNT);

  delete vertexData;
  return result;
}

BBox* VertexBlockData::getSlabRawBBox(Block* t_block) {
  Vec4 pos;
  GetXYZFromPos(&t_block->offset, &pos);
  const auto t_terrain = CrossCraft_World_GetMapPtr();
  const SlabOrientation orientation =
      GetSlabOrientationDataFromMap(t_terrain, pos.x, pos.y, pos.z);

  const auto vertexData = orientation == SlabOrientation::Top
                              ? VertexBlockData::getTopSlabVertexData()
                              : VertexBlockData::getBottomSlabVertexData();
  auto result = new BBox(vertexData, VETEX_COUNT);

  delete vertexData;
  return result;
}

BBox* VertexBlockData::getRawBBoxByBlock(Block* t_block) {
  switch (t_block->type) {
    case Blocks::TORCH:
      return VertexBlockData::getTorchRawBBox();
      break;

    case Blocks::STONE_SLAB:
    case Blocks::BRICKS_SLAB:
    case Blocks::OAK_PLANKS_SLAB:
    case Blocks::SPRUCE_PLANKS_SLAB:
    case Blocks::BIRCH_PLANKS_SLAB:
    case Blocks::ACACIA_PLANKS_SLAB:
    case Blocks::STONE_BRICK_SLAB:
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
    case Blocks::CHISELED_STONE_BRICKS_SLAB:
      return VertexBlockData::getSlabRawBBox(t_block);
      break;

    default:
      return VertexBlockData::getCuboidRawBBox();
      break;
  }
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
