#pragma once
#include <tamtypes.h>
#include <vector>
#include <array>
#include <stdlib.h>
#include <string>
#include <codecvt>
#include <locale>
#include "tyra"
#include "managers/font/font_options.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::Vec2;

/**
 * @brief Declaration of global tick variables
 *
 */
extern std::string SpecialValidChars;
extern std::string UpperCaseAlphaChars;
extern std::string LowerCaseAlphaChars;
extern std::string AlphanumericValidChars;
extern std::string AlphaValidChars;
extern std::string NumericValidChars;
extern std::string AllValidChars;
extern std::array<Sprite*, 256> printable_ascii_chars_sprites;
extern u8 char_widths[256];
extern u8 LINE_FEED;
extern u8 NULL_CHAR;
extern u8 BACK_SPACE;
extern u8 BASE_LINE_HEIGHT;
extern Texture* Font_ASCII_Texture;

void FontManager_init(Renderer* t_renderer);

void FontManager_printText(const char* text, const FontOptions& options);
void FontManager_printText(const char* text, const float& x, const float& y);

void FontManager_printText(const std::string& text, const FontOptions& options);
void FontManager_printText(const std::string& text, const float& x,
                           const float& y);

float FontManager_calcLinePadding(const wchar_t* text, const size_t length,
                                  const TextAlignment alignment);

void FontManager_loadFontChars(Renderer* t_renderer);
void FontManager_unloadFontChars();
inline const u8 FontManager_getCodeFromChar(const wchar_t& c) {
  return int(c);
};
inline const Sprite* FontManager_getFontChatByCode(const u8& code);

// UTF-8
std::wstring FontManager_utf8_to_wstring(const std::string& str);
size_t FontManager_utf8len(char* s);

void FontManager_printText(const std::wstring& text, const FontOptions& options);
void FontManager_printText(const std::wstring& text, const float& x,
                           const float& y);

void FontManager_printText(const wchar_t* text, const size_t length,
                           const FontOptions& options);
