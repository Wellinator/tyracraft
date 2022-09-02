#pragma once
#include <tamtypes.h>

class Tool {
 public:
  Tool(){};
  virtual ~Tool(){};

 protected:
  u8 isEnchanted = 0;
  float durability = 100.0f;
};