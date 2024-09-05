#pragma once

#include <constants.hpp>
#include <stdint.h>
#include <stdbool.h>
#include <array>
#include <tyra>
#include <stdint-gcc.h>

using Tyra::Vec4;

class LevelMap {
 public:
  uint16_t width;
  uint16_t length;
  uint16_t height;

  uint16_t spawnX, spawnY, spawnZ;

  uint8_t blocks[OVERWORLD_SIZE];
  uint8_t lightData[OVERWORLD_SIZE];
  uint8_t metaData[OVERWORLD_SIZE];
};

class Level {
 public:
  LevelMap map;

  Level(int seed);

  uint8_t getBlockByWorldPosition(Vec4* pos);

  uint8_t GetMetaDataFromMap(uint16_t x, uint16_t y, uint16_t z);
  uint8_t SetMetaDataToMap(uint16_t x, uint16_t y, uint16_t z, uint8_t data);

  void SetLiquidOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                     const LiquidOrientation orientation);
  LiquidOrientation GetLiquidOrientationDataFromMap(uint16_t x, uint16_t y,
                                                    uint16_t z);

  void SetTorchOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                    const BlockOrientation orientation);
  BlockOrientation GetTorchOrientationDataFromMap(uint16_t x, uint16_t y,
                                                  uint16_t z);

  void SetBlockOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                    const BlockOrientation orientation);
  BlockOrientation GetBlockOrientationDataFromMap(uint16_t x, uint16_t y,
                                                  uint16_t z);

  void SetLiquidDataToMap(uint16_t x, uint16_t y, uint16_t z,
                          const u8 liquidLevel);
  u8 GetLiquidDataFromMap(uint16_t x, uint16_t y, uint16_t z);

  void SetSlabOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z,
                                   const SlabOrientation orientation);
  void ResetSlabOrientationDataToMap(uint16_t x, uint16_t y, uint16_t z);
  SlabOrientation GetSlabOrientationDataFromMap(uint16_t x, uint16_t y,
                                                uint16_t z);

  uint8_t GetLightDataFromMap(uint16_t x, uint16_t y, uint16_t z);
  uint8_t GetLightFromMap(uint16_t x, uint16_t y, uint16_t z);
  uint8_t GetBlockLightFromMap(uint16_t x, uint16_t y, uint16_t z);
  uint8_t GetSunLightFromMap(uint16_t x, uint16_t y, uint16_t z);
  uint8_t GetBlockFromMap(uint16_t x, uint16_t y, uint16_t z);
  uint8_t GetBlockFromMapByIndex(uint32_t index);

  uint8_t SafeGetBlockFromMap(uint16_t x, uint16_t y, uint16_t z);

  void SetBlockInMap(uint16_t x, uint16_t y, uint16_t z, uint8_t block);
  void SetBlockInMapByIndex(uint32_t index, uint8_t block);

  void SetBlockLightInMap(uint16_t x, uint16_t y, uint16_t z, uint16_t light);
  void SetSunLightInMap(uint16_t x, uint16_t y, uint16_t z, uint16_t light);

  bool BoundCheckMap(uint16_t x, uint16_t y, uint16_t z);

  static uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z);
  static void GetXYZFromPos(uint32_t pos, uint32_t* x, uint32_t* y,
                            uint32_t* z);
  static void GetXYZFromPos(u32* pos, Vec4* t_Offset);
};