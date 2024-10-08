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

  uint8_t blocks[OVERWORLD_SIZE];
  uint8_t lightData[OVERWORLD_SIZE];
  uint8_t metaData[OVERWORLD_SIZE];
} LevelMap;

typedef struct {
  LevelMap map;
} Level;

/**
 * This method should ONLY be used by a client in single-player or a server for
 * internal use.
 * @return Returns a pointer to the level
 */
Level* CrossCraft_World_GetLevelPtr();
LevelMap* CrossCraft_World_GetMapPtr();

uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z);

void GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y, uint32_t* z);
void GetXYZFromPos(u32* pos, Vec4* t_Offset);

uint8_t getBlockByWorldPosition(LevelMap* map, Vec4* pos);

uint8_t GetMetaDataFromMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z);
uint8_t SetMetaDataToMap(LevelMap* map, uint16_t x, uint16_t y, uint16_t z,
                         uint8_t data);

void SetLiquidOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                   uint16_t z,
                                   const LiquidOrientation orientation);
LiquidOrientation GetLiquidOrientationDataFromMap(LevelMap* map, uint16_t x,
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

void SetSlabOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                 uint16_t z, const SlabOrientation orientation);
void ResetSlabOrientationDataToMap(LevelMap* map, uint16_t x, uint16_t y,
                                   uint16_t z);
SlabOrientation GetSlabOrientationDataFromMap(LevelMap* map, uint16_t x,
                                              uint16_t y, uint16_t z);

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
