#pragma once
#include <tyra>
#include <string>
#include <inttypes.h>

using Tyra::Sprite;

class LanguageInfoModel {
 public:
  u8 id;
  std::string path;
  std::string title;
  std::string author;
  std::string revision;
  Sprite icon;
};
