#pragma once
#include <tamtypes.h>
#include <vector>
#include <string>
#include "tyra"
#include "managers/font/font_char.hpp"
#include "managers/font/font_options.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Vec2;

class FontManager {
 public:
  const std::string AlphanumericValidChars = std::string(
      "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  const std::string AlphaValidChars =
      std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  const std::string NumericValidChars = std::string("1234567890");

  FontManager();
  ~FontManager();

  void init(Renderer* renderer);

  void printText(const std::string& text, const FontOptions& options);

  void printText(const char* text, const FontOptions& options) {
    printText(std::string(text), options);
  }

  void printText(const char* text, const float& x, const float& y) {
    printText(std::string(text), FontOptions(Vec2(x, y)));
  }

 private:
  Renderer* t_renderer;
  FontChar* printable_ascii_chars_sprites[256];
  u8 char_widths[256] = {
      8,  6,  9,  6,  6,  6,  6,  6,  6,  96, 0,  6,  14, 0,  6,  6,  6,  6,
      6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  0,  0,  0,  0,  4,  4,  8,  12,
      12, 12, 12, 4,  10, 10, 10, 12, 4,  12, 4,  12, 12, 12, 12, 12, 12, 12,
      12, 12, 12, 12, 4,  4,  10, 12, 10, 12, 14, 12, 12, 12, 12, 12, 12, 12,
      12, 8,  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
      12, 8,  12, 8,  12, 12, 6,  12, 12, 12, 12, 12, 10, 12, 12, 4,  12, 10,
      6,  12, 12, 12, 12, 12, 12, 12, 8,  12, 12, 12, 12, 12, 12, 10, 4,  10,
      14, 4,  12, 0,  6,  12, 10, 12, 8,  8,  8,  16, 12, 6,  20, 0,  12, 0,
      0,  6,  6,  10, 10, 6,  6,  12, 12, 16, 12, 6,  20, 0,  12, 12, 3,  4,
      10, 12, 12, 12, 4,  12, 10, 16, 8,  10, 10, 4,  16, 12, 8,  12, 8,  8,
      6,  12, 12, 4,  8,  8,  8,  10, 18, 18, 18, 12, 12, 12, 12, 12, 12, 12,
      20, 12, 12, 12, 12, 12, 8,  8,  8,  8,  14, 12, 12, 12, 12, 12, 12, 10,
      12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 20, 12, 12, 12,
      12, 12, 4,  6,  6,  6,  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
      12, 12, 12, 12};
  const u8 LINE_FEED = 10;
  const u8 NULL_CHAR = 0;
  const u8 BACK_SPACE = 8;
  const u8 BASE_LINE_HEIGHT = 20;

  void loadFontChars();
  void unloadFontChars();
  inline const u8 getCodeFromChar(const char& c) { return int(c); };
  const FontChar* getFontChatByCode(const u8& code);
};
