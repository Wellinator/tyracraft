#pragma once

#include <constants.hpp>
#include <stdint.h>
#include <stdbool.h>
#include <array>
#include <tyra>
#include <stdint-gcc.h>

using Tyra::Vec4;

typedef struct {
  uint16_t width;
  uint16_t length;
  uint16_t height;

  uint16_t spawnX, spawnY, spawnZ;

  uint8_t* blocks;
  uint8_t* lightData;
  uint8_t* metaData;
} LevelMap;

typedef struct {
  LevelMap map;
} Level;

uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z);

void GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y, uint32_t* z);
void GetXYZFromPos(u32* pos, Vec4* t_Offset);

uint8_t getBlockByWorldPosition(LevelMap* map, Vec4* pos);

uint8_t GetMetaDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t SetMetaDataToMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                         uint8_t data);

void SetOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                             const BlockOrientation orientation);
BlockOrientation GetOrientationDataFromMap(LevelMap* map, uint16_t x,
                                           uint16_t y, uint16_t z);


void SetLiquidOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                   uint16_t z,
                                   const BlockOrientation orientation);
BlockOrientation GetLiquidOrientationDataFromMap(LevelMap* map, uint16_t x,
                                                 uint16_t y, uint16_t z);

void SetTorchOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                  uint16_t z,
                                  const BlockOrientation orientation);
BlockOrientation GetTorchOrientationDataFromMap(LevelMap* map, uint16_t x,
                                                uint16_t y, uint16_t z);

void SetBlockOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                  uint16_t z,
                                  const BlockOrientation orientation);
BlockOrientation GetBlockOrientationDataFromMap(LevelMap* map, uint16_t x,
                                                uint16_t y, uint16_t z);

void SetLiquidDataToMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                        const u8 liquidLevel);
u8 GetLiquidDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);

uint8_t GetLightDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetBlockLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetSunLightFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetBlockFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t GetBlockFromMapByIndex(LevelMap* map, uint32_t index);

void SetBlockInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                   uint8_t block);
void SetBlockInMapByIndex(LevelMap* map, uint32_t index, uint8_t block);

void SetBlockLightInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                        uint16_t light);
void SetSunLightInMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                      uint16_t light);

bool BoundCheckMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
