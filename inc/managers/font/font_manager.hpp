#pragma once
#include <tamtypes.h>
#include <vector>
#include <string>
#include "tyra"
#include "font_char.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Vec2;

class FontManager {
 public:
  FontManager(Renderer* renderer);
  ~FontManager();

  void printText(const std::string& text, const Vec2& position,
                 const Color& color = Color(255), const float& scale = 1.0F);

  void printText(const char* text, const Vec2& position,
                 const Color& color = Color(255), const float& scale = 1.0F) {
    printText(std::string(text), position, color, scale);
  }

  void printText(const char* text, const float& posX, const float& posY,
                 const Color& color = Color(255), const float& scale = 1.0F) {
    printText(std::string(text), Vec2(posX, posY), color, scale);
  }

 private:
  Renderer* t_renderer;
  std::vector<FontChar*> printable_ascii_chars_sprites;
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

  void loadFontChars();
  void unloadFontChars();
  inline const u8 getCodeFromChar(const char& c) { return int(c); };
  const FontChar* getFontChatByCode(const u8& code);
};
