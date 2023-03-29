#pragma once
#include <tamtypes.h>
#include <vector>
#include <array>
#include <string>
#include "tyra"
#include "managers/font/font_options.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
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

void FontManager_init(Renderer* t_renderer);
void FontManager_printText(const std::string& text, const FontOptions& options);
void FontManager_printText(const char* text, const FontOptions& options);
void FontManager_printText(const char* text, const float& x, const float& y);
void FontManager_loadFontChars(Renderer* t_renderer);
void FontManager_unloadFontChars();
inline const u8 FontManager_getCodeFromChar(const char& c) { return int(c); };
inline const Sprite* FontManager_getFontChatByCode(const u8& code);
