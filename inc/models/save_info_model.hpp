#pragma once
#include <tyra>
#include <string>
#include <inttypes.h>
#include <ctime>

using Tyra::Sprite;

class SaveInfoModel {
 public:
  u8 id;
  u8 version;
  u8 valid;
  std::string path;
  std::string name;
  std::string createdAt;
  time_t timestamp;
  Sprite icon;
};
