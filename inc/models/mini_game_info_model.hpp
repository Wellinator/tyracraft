#pragma once
#include <tyra>
#include <string>
#include <inttypes.h>

using Tyra::Sprite;

class MiniGameInfoModel {
 public:
  u8 id;
  std::string title;
  std::string description;
  Sprite icon;
};
