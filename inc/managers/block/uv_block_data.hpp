#pragma once
#include <tamtypes.h>
#include "tyra"

using Tyra::Vec4;

class UVBlockData {
 public:
  UVBlockData();

  ~UVBlockData();

  static const Vec4* getUVMap();
};
