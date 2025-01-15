#pragma once
#include <tamtypes.h>
#include <vector>
#include <array>
#include <stdlib.h>
#include <string>
#include <codecvt>
#include <locale>
#include "tyra"
#include "singleton.hpp"
#include "managers/font/font_options.hpp"

#define LINE_FEED 10
#define NULL_CHAR 0
#define BACK_SPACE 8
#define BASE_LINE_HEIGHT 20

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::Vec2;

class FontManager : public Singleton<FontManager> {
 public:
  std::string SpecialValidChars = " -_";
  std::string UpperCaseAlphaChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::string LowerCaseAlphaChars = "abcdefghijklmnopqrstuvwxyz";
  std::string NumericValidChars = "0123456789";
  std::string AlphaValidChars =
      std::string(UpperCaseAlphaChars + LowerCaseAlphaChars);
  std::string AlphanumericValidChars =
      std::string(NumericValidChars + AlphaValidChars);
  std::string AllValidChars =
      std::string(SpecialValidChars + AlphanumericValidChars);

  std::array<Sprite*, 256> printable_ascii_chars_sprites;

  Texture* Font_ASCII_Texture;

  FontManager(Renderer* t_renderer);
  ~FontManager();

  void init();

  void printText(const char* text, const FontOptions& options);
  void printText(const char* text, const float& x, const float& y);

  void printText(const std::string& text, const FontOptions& options);
  void printText(const std::string& text, const float& x, const float& y);

  void printText(const std::wstring& text, const FontOptions& options);
  void printText(const std::wstring& text, const float& x, const float& y);

  void printText(const wchar_t* text, const size_t length,
                 const FontOptions& options);

 private:
  Renderer* pRenderer = nullptr;

  const u8 char_widths[256] = {
      10, 8, 11, 8, 8, 8, 8, 8, 8, 98, 2, 8, 16, 2, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 2, 2, 2, 2, 6, 6, 10, 14, 14, 14, 14, 6, 12, 12, 12, 14, 6,
      14, 6, 14,

      // index 47 - 56 are number from 0 to 9;
      14, 14, 14, 14, 14, 14, 14, 14, 14, 14,

      //
      6, 6, 12, 14, 12, 14, 16,

      // index 64 - 89 are number upper case letter from A to Z;
      14, 14, 14, 14, 14, 14, 14, 14, 12, 14, 14, 14, 14, 14, 14, 14, 14, 14,
      14, 14, 14, 14, 14, 14, 14, 14,

      //
      10, 14, 10, 14, 14, 10,

      // index 64 - 89 are number lower case letter from a to z;
      14, 14, 14, 14, 12, 14, 14, 12, 12, 12, 12, 10, 14, 14, 14, 14, 12, 12,
      14, 12, 14, 12, 14, 14, 14, 14,

      //
      12, 6, 12, 16, 6, 14, 2, 8, 14, 12, 14, 10, 10, 10, 18, 14, 8, 22, 2, 14,
      2, 2, 8, 8, 12, 12, 8, 8, 14, 14, 18, 14, 8, 22, 2, 14, 14, 5, 6, 12, 14,
      14, 14, 6, 14, 12, 18, 10, 12, 12, 6, 18, 14, 10, 14, 10, 10, 8, 14, 14,
      6, 10, 10, 10, 12, 20, 20, 20, 14, 14, 14, 14, 14, 14, 14, 22, 14, 14, 14,
      14, 14, 10, 10, 10, 10, 16, 14, 14, 14, 14, 14, 14, 12, 14, 14, 14, 14,
      14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 22, 14, 14, 14, 14, 14, 6, 8, 8,
      8, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14};

  float calcLinePadding(const wchar_t* text, const size_t length,
                        const TextAlignment alignment);
  void loadFontChars();
  void unloadFontChars();

  // UTF-8
  std::wstring utf8_to_wstring(const std::string& str);
  size_t utf8len(char* s);
};
