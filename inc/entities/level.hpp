#pragma once

#include <constants.hpp>
#include "singleton.hpp"
#include <stdint.h>
#include <stdbool.h>
#include <array>
#include <tyra>
#include <stdint-gcc.h>

using Tyra::Vec4;

class LevelIntersectQueryResult {
 public:
  Vec4 offset;
  Vec4 hitPosition;
  uint8_t blockType;
};

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

class Level : public Singleton<Level> {
 public:
  LevelMap map;

  Level(int seed);

  uint8_t getBlockByWorldPosition(const Vec4* pos);
  Vec4 roundToBlockCenter(const Vec4& pos);
  Vec4 worldPosToOffset(const Vec4& pos);
  Vec4 worldPosToOffsetNotRounded(const Vec4& pos);
  Vec4 offsetToWorldPos(const Vec4& offset);
  Vec4 offsetToWorldPos(const Vec4* offset);

  bool isPositionEmpty(const Vec4& pos) {
    return isPositionEmpty(pos.x, pos.y, pos.z);
  }
  bool isPositionEmpty(const uint16_t& x, const uint16_t& y, const uint16_t& z);

  bool isReplaceableBySolidBlock(const Vec4& pos) {
    return isReplaceableBySolidBlock(pos.x, pos.y, pos.z);
  }
  bool isReplaceableBySolidBlock(const uint16_t& x, const uint16_t& y,
                                 const uint16_t& z);

  bool isGrassAtPosition(const uint16_t& x, const uint16_t& y,
                         const uint16_t& z);

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
  uint8_t GetBlockFromMap(Vec4* offset);
  uint8_t GetBlockFromMapByIndex(uint32_t index);

  uint8_t SafeGetBlockFromMap(uint16_t x, uint16_t y, uint16_t z);

  void SetBlockInMap(uint16_t x, uint16_t y, uint16_t z, uint8_t block);
  void SetBlockInMapByIndex(uint32_t index, uint8_t block);

  void SetBlockLightInMap(uint16_t x, uint16_t y, uint16_t z, uint16_t light);
  void SetSunLightInMap(uint16_t x, uint16_t y, uint16_t z, uint16_t light);

  bool BoundCheckMap(uint16_t x, uint16_t y, uint16_t z);

  uint32_t OffsetToIndex(const Vec4& offset);

  static uint32_t GetPosFromXYZ(uint32_t x, uint32_t y, uint32_t z);

  /**
   * Get start and end points of the line segment and return all intersected
   * blocks
   * @details This function performs a 3D line intersection test with the level
   * grid using DDA algorithm. It calculates the start and end points of the
   * line segment based on the provided start and
   * @param start Start point of the line segment
   * @param end End point of the line segment
   * @param pResults Pointer to vector of LevelIntersectQueryResult
   */
  void getIntersectedBlocks(const Vec4& start, const Vec4& end,
                            std::vector<LevelIntersectQueryResult>* pResults);
};