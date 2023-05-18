#include "entities/level.hpp"

// Gets the position in the data array from the given x, y, and z coordinates.
uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z) {
  return x + (y << 10) + (z << 20);
}

// Gets the x, y, and z coordinates from the given position in the data array.
void GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y, uint32_t* z) {
  *x = pos % 1024;
  *y = (pos >> 10) % 1024;
  *z = (pos >> 20) % 1024;
}

// Gets the data value at the given coordinates in the map.
uint8_t GetDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->data[index];
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

// Sets the block ID at the given coordinates in the map.
void SetBlockInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                   uint8_t block) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  map->blocks[index] = block;
}

uint8_t GetBlockLightFromMap(LevelMap* map, uint16_t x, uint16_t y,
                             uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return map->lightData[index] & 0x0F;
}

uint8_t GetSunLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z) {
  uint32_t index = (y * map->length * map->width) + (z * map->width) + x;
  return (map->lightData[index] & 0xF0);
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
