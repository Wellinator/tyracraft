#pragma once
#include <tyra>
#include <string>
#include <inttypes.h>

using Tyra::Sprite;

class SaveInfoModel {
 public:
  u8 id;
  std::string path;
  std::string title;
  std::string createdAt;
  Sprite icon;
};
