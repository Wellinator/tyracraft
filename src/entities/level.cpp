#include "entities/level.hpp"
#include <cfloat>
#include <cmath>

Level::Level(int seed) : Singleton<Level>() {
  TYRA_LOG("Generating base level template");
  srand(seed);

  map.width = OVERWORLD_H_DISTANCE;
  map.length = OVERWORLD_H_DISTANCE;
  map.height = OVERWORLD_V_DISTANCE;

  // TODO: move to player class
  map.spawnX = 128;
  map.spawnY = 59;
  map.spawnZ = 128;

  // For some reason I need to clear the array garbage
  // I was initialized with new key word, very weird!
  for (size_t i = 0; i < OVERWORLD_SIZE; i++) {
    map.blocks[i] = 0;
    map.lightData[i] = 0;
    map.metaData[i] = 0;
  }
}

// Gets the position in the data array from the given x, y, and z coordinates.
uint32_t Level::GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z) {
  return x + (y << 7) + (z << 14);
}

// Gets the x, y, and z coordinates from the given position in the data array.
void Level::GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y, uint32_t* z) {
  *x = pos % OVERWORLD_H_DISTANCE;
  *y = (pos >> 7) % OVERWORLD_V_DISTANCE;
  *z = (pos >> 14) % OVERWORLD_H_DISTANCE;
}

// Gets the x, y, and z coordinates from the given position in the data array.
void Level::GetXYZFromPos(u32* pos, Vec4* t_Offset) {
  t_Offset->x = *pos % OVERWORLD_H_DISTANCE;
  t_Offset->y = (*pos >> 7) % OVERWORLD_V_DISTANCE;
  t_Offset->z = (*pos >> 14) % OVERWORLD_H_DISTANCE;
}

// Gets the metadata value at the given coordinates in the map.
uint8_t Level::GetMetaDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return map.metaData[index];
}

// Sets the metadata value at the given coordinates in the map.
uint8_t Level::SetMetaDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                uint8_t data) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return map.metaData[index] = data;
}

void Level::SetLiquidOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                          const LiquidOrientation orientation) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;

  // value = (value & ~mask) | (newvalue & mask);
  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) << 5 & LIQUID_ORIENTATION_MASK;

  const u8 _setedValue =
      (map.metaData[index] & ~LIQUID_ORIENTATION_MASK) | newvalue;

  map.metaData[index] = _setedValue;
}

void Level::SetTorchOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                         const BlockOrientation orientation) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;

  // value = (value & ~mask) | (newvalue & mask);
  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) & TORCH_ORIENTATION_MASK;

  const u8 _setedValue =
      (map.metaData[index] & ~TORCH_ORIENTATION_MASK) | newvalue;

  map.metaData[index] = _setedValue;
}

void Level::SetBlockOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                         const BlockOrientation orientation) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;

  // value = (value & ~mask) | (newvalue & mask);
  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) & BLOCK_ORIENTATION_MASK;

  const u8 _setedValue =
      (map.metaData[index] & ~BLOCK_ORIENTATION_MASK) | newvalue;

  map.metaData[index] = _setedValue;
}

void Level::SetSlabOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                        const SlabOrientation orientation) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;

  // value = (value & ~mask) | (newvalue & mask);
  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) << 2 & SLAB_ORIENTATION_MASK;

  const u8 _setedValue =
      (map.metaData[index] & ~SLAB_ORIENTATION_MASK) | newvalue;

  map.metaData[index] = _setedValue;
}

void Level::ResetSlabOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;

  // value = (value & ~mask) | (newvalue & mask);
  const uint8_t newvalue = 0 & SLAB_ORIENTATION_MASK;
  const u8 _setedValue =
      (map.metaData[index] & ~SLAB_ORIENTATION_MASK) | newvalue;

  map.metaData[index] = _setedValue;
}

LiquidOrientation Level::GetLiquidOrientationDataFromMap(uint16_t x, uint16_t y,
                                                         uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  const uint8_t response = map.metaData[index] & LIQUID_ORIENTATION_MASK;
  return static_cast<LiquidOrientation>(response >> 5);
}

BlockOrientation Level::GetTorchOrientationDataFromMap(uint16_t x, uint16_t y,
                                                       uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  const uint8_t response = map.metaData[index] & TORCH_ORIENTATION_MASK;
  return static_cast<BlockOrientation>(response);
}

BlockOrientation Level::GetBlockOrientationDataFromMap(uint16_t x, uint16_t y,
                                                       uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  const uint8_t response = map.metaData[index] & BLOCK_ORIENTATION_MASK;
  return static_cast<BlockOrientation>(response);
}

SlabOrientation Level::GetSlabOrientationDataFromMap(uint16_t x, uint16_t y,
                                                     uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  const uint8_t response = map.metaData[index] & SLAB_ORIENTATION_MASK;
  return static_cast<SlabOrientation>(response >> 2);
}

// Set the liquid metadata value at the given coordinates in the map.
void Level::SetLiquidDataToMap(uint16_t x, uint16_t y, uint16_t z,
                               const u8 liquidLevel) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;

  const uint8_t newvalue = liquidLevel << 2 & LIQUID_LEVEL_MASK;
  const u8 _setedValue = (map.metaData[index] & ~LIQUID_LEVEL_MASK) | newvalue;
  // printf("Setted: %i", _setedValue >> 2);
  map.metaData[index] = _setedValue;
}

// Gets the liquid metadata value at the given coordinates in the map.
u8 Level::GetLiquidDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  const uint8_t response = map.metaData[index] & LIQUID_LEVEL_MASK;
  return (response >> 2);
}

// Gets the light data value at the given coordinates in the map.
uint8_t Level::GetLightDataFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return map.lightData[index];
}

// Gets the light value at the given coordinates in the map.
uint8_t Level::GetLightFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  uint8_t v = map.lightData[index];
  uint8_t res = ((v & 0xF0) >> 4) + (v & 0x0F);

  if (res > 0x0F) return 0x0F;
  return res;
}

// Gets the block ID at the given coordinates in the map.
uint8_t Level::GetBlockFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return map.blocks[index];
}

uint8_t Level::GetBlockFromMap(Vec4* offset) {
  uint32_t x = static_cast<uint32_t>(offset->x);
  uint32_t y = static_cast<uint32_t>(offset->y);
  uint32_t z = static_cast<uint32_t>(offset->z);

  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return map.blocks[index];
}

uint8_t Level::SafeGetBlockFromMap(uint16_t x, uint16_t y, uint16_t z) {
  return BoundCheckMap(x, y, z) ? GetBlockFromMap(x, y, z)
                                : (uint8_t)Blocks::VOID;
}

// Gets the block ID at the given coordinates in the map.
uint8_t Level::GetBlockFromMapByIndex(uint32_t index) {
  return map.blocks[index];
}

// Sets the block ID at the given coordinates in the map.
void Level::SetBlockInMap(uint16_t x, uint16_t y, uint16_t z, uint8_t block) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  map.blocks[index] = block;
}

// Sets the block ID at the given coordinates in the map.
void Level::SetBlockInMapByIndex(uint32_t index, uint8_t block) {
  map.blocks[index] = block;
}

uint8_t Level::GetBlockLightFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return map.lightData[index] & 0x0F;
}

uint8_t Level::GetSunLightFromMap(uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  return ((map.lightData[index] >> 4) & 0xF);
}

void Level::SetBlockLightInMap(uint16_t x, uint16_t y, uint16_t z,
                               uint16_t light) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  map.lightData[index] = (map.lightData[index] & 0xF0) | light;
}

void Level::SetSunLightInMap(uint16_t x, uint16_t y, uint16_t z,
                             uint16_t light) {
  uint32_t index = (y * map.length * map.width) + (z * map.width) + x;
  map.lightData[index] = (map.lightData[index] & 0x0F) | (light << 4);
}

// Returns true if the given coordinates are within the bounds of the map.
bool Level::BoundCheckMap(uint16_t x, uint16_t y, uint16_t z) {
  return (x < map.length && y < map.height && z < map.width);
}

uint8_t Level::getBlockByWorldPosition(const Vec4* pos) {
  Vec4 result = *pos / (DOUBLE_BLOCK_SIZE);
  auto x = static_cast<uint16_t>(result.x);
  auto y = static_cast<uint16_t>(result.y);
  auto z = static_cast<uint16_t>(result.z);

  return BoundCheckMap(x, y, z) ? GetBlockFromMap(x, y, z) : 0;
}

Vec4 Level::worldPosToOffset(const Vec4& pos) {
  Vec4 offset = pos / DOUBLE_BLOCK_SIZE;
  return Vec4(std::round(offset.x), std::round(offset.y), std::round(offset.z));
}

Vec4 Level::offsetToWorldPos(const Vec4* offset) {
  return (*offset) * DOUBLE_BLOCK_SIZE;
}

Vec4 Level::offsetToWorldPos(const Vec4& offset) {
  return offset * DOUBLE_BLOCK_SIZE;
}

Vec4 Level::roundToBlockCenter(const Vec4& pos) {
  return worldPosToOffset(pos) * DOUBLE_BLOCK_SIZE;
}

void Level::getIntersectedBlocks(
    const Vec4& start, const Vec4& end,
    std::vector<LevelIntersectQueryResult>* pResults) {
  Vec4 startOffset = worldPosToOffset(start);
  Vec4 endOffset = worldPosToOffset(end);

  // Clear results vector
  pResults->clear();

  // Current position
  int x = (int)startOffset.x;
  int y = (int)startOffset.y;
  int z = (int)startOffset.z;

  // Direction and step
  Vec4 delta = endOffset - startOffset;
  int stepX = delta.x > 0 ? 1 : -1;
  int stepY = delta.y > 0 ? 1 : -1;
  int stepZ = delta.z > 0 ? 1 : -1;

  // Calculate t delta values for DDA
  float tDeltaX = delta.x != 0 ? std::abs(1.0f / delta.x) : FLT_MAX;
  float tDeltaY = delta.y != 0 ? std::abs(1.0f / delta.y) : FLT_MAX;
  float tDeltaZ = delta.z != 0 ? std::abs(1.0f / delta.z) : FLT_MAX;

  // Calculate initial t values
  float tMaxX = delta.x != 0 ? (stepX > 0 ? (x + 1 - startOffset.x)
                                          : (startOffset.x - x)) *
                                   tDeltaX
                             : FLT_MAX;
  float tMaxY = delta.y != 0 ? (stepY > 0 ? (y + 1 - startOffset.y)
                                          : (startOffset.y - y)) *
                                   tDeltaY
                             : FLT_MAX;
  float tMaxZ = delta.z != 0 ? (stepZ > 0 ? (z + 1 - startOffset.z)
                                          : (startOffset.z - z)) *
                                   tDeltaZ
                             : FLT_MAX;

  // DDA traversal
  int maxIterations =
      OVERWORLD_H_DISTANCE + OVERWORLD_V_DISTANCE + OVERWORLD_H_DISTANCE;
  int iterations = 0;

  while (iterations < maxIterations) {
    // Check bounds and add current block
    if (x >= 0 && y >= 0 && z >= 0 && x < OVERWORLD_H_DISTANCE &&
        y < OVERWORLD_V_DISTANCE && z < OVERWORLD_H_DISTANCE) {
      // Calculate hit position as the intersection point with the current voxel
      Vec4 blockOffset(x, y, z);
      u8 blockType = GetBlockFromMap(x, y, z);
      if (blockType > (u8)Blocks::AIR_BLOCK) {
        Vec4 hitPos = offsetToWorldPos(blockOffset);
        pResults->emplace_back(
            LevelIntersectQueryResult{blockOffset, hitPos, blockType});
      }
    }

    // Check if we've reached the end
    if (x == (int)endOffset.x && y == (int)endOffset.y && z == (int)endOffset.z)
      break;

    // Move to next voxel
    if (tMaxX < tMaxY && tMaxX < tMaxZ) {
      x += stepX;
      tMaxX += tDeltaX;
    } else if (tMaxY < tMaxZ) {
      y += stepY;
      tMaxY += tDeltaY;
    } else {
      z += stepZ;
      tMaxZ += tDeltaZ;
    }

    iterations++;
  }
}