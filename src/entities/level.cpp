#include "entities/level.hpp"

// Gets the position in the data array from the given x, y, and z coordinates.
uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z) {
  return x + (y << 7) + (z << 14);
}

// Gets the x, y, and z coordinates from the given position in the data array.
void GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y, uint32_t* z) {
  *x = pos % OVERWORLD_H_DISTANCE;
  *y = (pos >> 7) % OVERWORLD_V_DISTANCE;
  *z = (pos >> 14) % OVERWORLD_H_DISTANCE;
}

// Gets the x, y, and z coordinates from the given position in the data array.
void GetXYZFromPos(u32* pos, Vec4* t_Offset) {
  t_Offset->x = *pos % OVERWORLD_H_DISTANCE;
  t_Offset->y = (*pos >> 7) % OVERWORLD_V_DISTANCE;
  t_Offset->z = (*pos >> 14) % OVERWORLD_H_DISTANCE;
}

// Gets the metadata value at the given coordinates in the map.
uint8_t GetMetaDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->metaData[index];
}

// Sets the metadata value at the given coordinates in the map.
uint8_t SetMetaDataToMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                         uint8_t data) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->metaData[index] = data;
}

// Gets the metadata value at the given coordinates in the map.
uint8_t SetOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                uint16_t z, BlockOrientation orientation) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;

  // value = (value & ~mask) | (newvalue & mask);
  const uint8_t newvalue =
      static_cast<uint8_t>(orientation) & BLOCK_ORIENTATION_MASK;

  const u8 _setedValue =
      (map->metaData[index] & ~BLOCK_ORIENTATION_MASK) | newvalue;
  printf("Setted: %i", _setedValue);
  return map->metaData[index] = _setedValue;
}

// Gets the metadata value at the given coordinates in the map.
BlockOrientation GetOrientationDataFromMap(LevelMap* map, uint16_t x,
                                           uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  const uint8_t response = map->metaData[index] & BLOCK_ORIENTATION_MASK;
  return static_cast<BlockOrientation>(response);
}

// Gets the light data value at the given coordinates in the map.
uint8_t GetLightDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->lightData[index];
}

// Gets the light value at the given coordinates in the map.
uint8_t GetLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  uint8_t v = map->lightData[index];
  uint8_t res = ((v & 0xF0) >> 4) + (v & 0x0F);

  if (res > 0x0F) return 0x0F;
  return res;
}

// Gets the block ID at the given coordinates in the map.
uint8_t GetBlockFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->blocks[index];
}

// Gets the block ID at the given coordinates in the map.
uint8_t GetBlockFromMapByIndex(LevelMap* map, uint32_t index) {
  return map->blocks[index];
}

// Sets the block ID at the given coordinates in the map.
void SetBlockInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                   uint8_t block) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  map->blocks[index] = block;
}

// Sets the block ID at the given coordinates in the map.
void SetBlockInMapByIndex(LevelMap* map, uint32_t index, uint8_t block) {
  map->blocks[index] = block;
}

uint8_t GetBlockLightFromMap(LevelMap* map, uint16_t x, uint16_t y,
                             uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->lightData[index] & 0x0F;
}

uint8_t GetSunLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return ((map->lightData[index] >> 4) & 0xF);
}

void SetBlockLightInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                        uint16_t light) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  map->lightData[index] = (map->lightData[index] & 0xF0) | light;
}

void SetSunLightInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                      uint16_t light) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  map->lightData[index] = (map->lightData[index] & 0x0F) | (light << 4);
}

// Returns true if the given coordinates are within the bounds of the map.
bool BoundCheckMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  return (x < map->length && y < map->height && z < map->width);
}

uint8_t getBlockByWorldPosition(LevelMap* map, Vec4* pos) {
  Vec4 result = *pos / (DUBLE_BLOCK_SIZE);
  auto x = static_cast<uint16_t>(result.x);
  auto y = static_cast<uint16_t>(result.y);
  auto z = static_cast<uint16_t>(result.z);

  return BoundCheckMap(map, x, y, z) ? GetBlockFromMap(map, x, y, z) : 0;
}