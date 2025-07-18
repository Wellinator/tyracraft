#pragma once
#include <tamtypes.h>
#include "tyra"
#include "singleton.hpp"
#include "constants.hpp"
#include "entities/level.hpp"
#include "./block_manager.hpp"

class VisibleFacesManager : public Singleton<VisibleFacesManager> {
 public:
  VisibleFacesManager();
  ~VisibleFacesManager();

  u8 getVisibleFacesByOffset(const Vec4& offset);

 private:
  u8 getLiquidBlockVisibleFaces(const Vec4& offset);
  u8 getSlabVisibleFaces(const Vec4& offset);
  u8 getLeavesVisibleFaces(const Vec4& offset);
  u8 getDefaultVisibleFaces(const Vec4& offset);

  inline u8 isTopFaceVisible(const Vec4* t_blockOffset);
  inline u8 isBottomFaceVisible(const Vec4* t_blockOffset);
  inline u8 isLeftFaceVisible(const Vec4* t_blockOffset);
  inline u8 isRightFaceVisible(const Vec4* t_blockOffset);
  inline u8 isFrontFaceVisible(const Vec4* t_blockOffset);
  inline u8 isBackFaceVisible(const Vec4* t_blockOffset);

  inline u8 isBlockTransparentAtPosition(const uint16_t x, const uint16_t y, const uint16_t z);
};
