#pragma once

#include <constants.hpp>
#include <stdint.h>
#include <stdbool.h>
#include <array>
#include <tyra>

using Tyra::Vec4;

typedef struct {
  uint16_t width;
  uint16_t length;
  uint16_t height;

  uint16_t spawnX, spawnY, spawnZ;

  uint8_t* blocks;
  uint8_t* lightData;

  // block data << 4
  // light data & 0x0F
  uint8_t* data;

} LevelMap;

typedef struct {
  LevelMap map;
} Level;

uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z);

void GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y, uint32_t* z);

uint8_t getBlockByWorldPosition(LevelMap* map, Vec4* pos);

uint8_t GetDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetBlockLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetSunLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetBlockFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);

void SetBlockInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                   uint8_t block);
void SetBlockLightInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                        uint16_t light);
void SetSunLightInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                      uint16_t light);

bool BoundCheckMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
