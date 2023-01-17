#pragma once
#include <tyra>
#include <string>
#include <inttypes.h>

using Tyra::Sprite;

class TexturePackInfoModel {
 public:
  TexturePackInfoModel(){};
  ~TexturePackInfoModel(){};

  u8 id;
  std::string path;
  std::string author;
  std::string title;
  std::string description;
  Sprite icon;
};
