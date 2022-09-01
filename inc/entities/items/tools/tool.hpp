#pragma once
#include <tamtypes.h>

class Tool {
  Tool();
  virtual ~Tool();

 protected:
  u8 isEnchanted = 0;
  float durability = 100.0f;
};