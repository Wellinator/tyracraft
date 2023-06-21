#pragma once
#include <tyra>
#include <string>
#include <inttypes.h>

using Tyra::Sprite;

class LanguageInfoModel {
 public:
  u8 id;
  std::string code;
  std::string path;
  std::string title;
  std::string author;
  std::string revision;
  std::string selectLabel;
  Sprite icon;
};
