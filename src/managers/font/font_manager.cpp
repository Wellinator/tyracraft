#include "managers/font/font_manager.hpp"

/**
 * @brief Definition of global tick variables
 *
 */
u8 LINE_FEED = 10;
u8 NULL_CHAR = 0;
u8 BACK_SPACE = 8;
u8 BASE_LINE_HEIGHT = 20;
std::string SpecialValidChars = std::string(" -_");
std::string NumericValidChars = std::string("1234567890");
std::string UpperCaseAlphaChars = std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
std::string LowerCaseAlphaChars = std::string("abcdefghijklmnopqrstuvwxyz");
Texture* Font_ASCII_Texture = nullptr;

std::string AlphaValidChars =
    std::string(UpperCaseAlphaChars + LowerCaseAlphaChars);
std::string AlphanumericValidChars =
    std::string(NumericValidChars + AlphaValidChars);
std::string AllValidChars =
    std::string(SpecialValidChars + AlphanumericValidChars);

Renderer* g_t_renderer = nullptr;
std::array<Sprite*, 256> printable_ascii_chars_sprites;
u8 char_widths[256] = {
    8, 6, 9, 6, 6, 6, 6, 6, 6, 96, 0, 6, 14, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 0, 0, 0, 0, 4, 4, 8, 12, 12, 12, 12, 4, 10, 10, 10, 12, 4, 12, 4,
    12,

    // index 47 - 56 are number from 0 to 9;
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    //

    4, 4, 10, 12, 10, 12, 14,

    // index 64 - 89 are number upper case letter from A to Z;
    12, 12, 12, 12, 12, 12, 12, 12, 10, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12,
    //

    8, 12, 8, 12, 12, 8,

    // index 64 - 89 are number lower case letter from a to z;
    12, 12, 12, 12, 10, 12, 12, 10, 10, 10, 10, 8, 12, 12, 12, 12, 10, 10, 12,
    10, 12, 10, 12, 12, 12, 12,
    //

    10, 4, 10, 14, 4, 12, 0, 6, 12, 10, 12, 8, 8, 8, 16, 12, 6, 20, 0, 12, 0, 0,
    6, 6, 10, 10, 6, 6, 12, 12, 16, 12, 6, 20, 0, 12, 12, 3, 4, 10, 12, 12, 12,
    4, 12, 10, 16, 8, 10, 10, 4, 16, 12, 8, 12, 8, 8, 6, 12, 12, 4, 8, 8, 8, 10,
    18, 18, 18, 12, 12, 12, 12, 12, 12, 12, 20, 12, 12, 12, 12, 12, 8, 8, 8, 8,
    14, 12, 12, 12, 12, 12, 12, 10, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 20, 12, 12, 12, 12, 12, 4, 6, 6, 6, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12};

void FontManager_init(Renderer* t_renderer) {
  g_t_renderer = t_renderer;
  FontManager_loadFontChars(t_renderer);
}

// void FontManager_printText(const std::string& text,
//                            const FontOptions& options) {
//   FontManager_printText(text.c_str(), text.length(), options);
// };

void FontManager_printText(const char* text, const FontOptions& options) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  std::wstring tempWstr = converter.from_bytes(text, text + strlen(text));
  FontManager_printText(tempWstr.c_str(), tempWstr.length(), options);
}

void FontManager_printText(const char* text, const float& x, const float& y) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  std::wstring tempWstr = converter.from_bytes(text, text + strlen(text));
  FontManager_printText(tempWstr.c_str(), tempWstr.length(),
                        FontOptions(Vec2(x, y)));
}

// void FontManager_printText(const std::string& text, const float& x,
//                            const float& y) {
//   FontManager_printText(text.c_str(), text.size(), FontOptions(Vec2(x, y)));
// }

void FontManager_printText(const wchar_t* text, const size_t length,
                           const FontOptions& options) {
  float cursorX = 0.0F;
  float cursorY = 0.0F;
  float padding = FontManager_calcLinePadding(text, length, options.alignment);
  padding *= options.scale;

  for (size_t i = 0; i < length; i++) {
    const u8 charCode = FontManager_getCodeFromChar(text[i]);
    const Sprite* fontCharAt = FontManager_getFontChatByCode(charCode);

    if (fontCharAt != nullptr) {
      // Break line
      if (charCode == LINE_FEED) {
        cursorY += BASE_LINE_HEIGHT * options.scale;
        cursorX = 0;
        continue;
      }

      Sprite spriteToPrint = *fontCharAt;
      spriteToPrint.position.set(options.position.x + cursorX - padding,
                                 options.position.y + cursorY);
      spriteToPrint.color.set(options.color);
      spriteToPrint.scale = options.scale;

      g_t_renderer->renderer2D.render(spriteToPrint);
      cursorX += (char_widths[charCode] + 2) * options.scale;
    } else {
      TYRA_TRAP(std::string("Char at ")
                    .append(std::to_string(i))
                    .append(std::string("("))
                    .append(std::to_string(charCode))
                    .append(std::string(") is not a valid ASCII code!"))
                    .c_str());
    }
  }
}

float FontManager_calcLinePadding(const wchar_t* text, const size_t length,
                                  const TextAlignment alignment) {
  float padding = 0.0F;

  if (alignment == TextAlignment::Left)
    return padding;
  else {
    const size_t stringLenth = length;
    for (size_t i = 0; i < stringLenth; i++) {
      const u8 charCode = FontManager_getCodeFromChar(text[i]);
      padding += (char_widths[charCode] + 2);
    }
  }

  return alignment == TextAlignment::Center ? padding / 2 : padding;
}

void FontManager_loadFontChars(Renderer* t_renderer) {
  const int MAX_COLS = 16;
  const u8 INITIAL_CHAR_CODE = 0;
  const u8 FINAL_CHAR_CODE = 255;

  Font_ASCII_Texture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/font/ascii.png"));

  for (size_t code = INITIAL_CHAR_CODE; code <= FINAL_CHAR_CODE; code++) {
    Sprite* charSprite = new Sprite();

    charSprite->mode = Tyra::MODE_REPEAT;
    charSprite->size.set(32.0F, 32.0F);

    const u8 x_offset = code < MAX_COLS ? code : code % MAX_COLS;
    const u8 y_offset = code < MAX_COLS ? 0 : std::floor(code / MAX_COLS);
    charSprite->offset = Vec2(x_offset, y_offset) * charSprite->size;

    Font_ASCII_Texture->addLink(charSprite->id);

    printable_ascii_chars_sprites[code] = charSprite;
  }
};

void FontManager_unloadFontChars() {
  for (size_t i = 0; i < 256; i++) {
    delete printable_ascii_chars_sprites[i];
  }
}

const Sprite* FontManager_getFontChatByCode(const u8& code) {
  return printable_ascii_chars_sprites[code];
}

// UTF-8
std::wstring FontManager_utf8_to_wstring(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.from_bytes(str);
}

size_t FontManager_utf8len(char* s) {
  size_t len = 0;
  for (; *s; ++s)
    if ((*s & 0xC0) != 0x80) ++len;
  return len;
}

void FontManager_printText(const std::string& text,
                           const FontOptions& options) {
  std::wstring tempWstr = FontManager_utf8_to_wstring(text);
  FontManager_printText(tempWstr.c_str(), tempWstr.length(), options);
};

void FontManager_printText(const std::string& text, const float& x,
                           const float& y) {
  std::wstring tempWstr = FontManager_utf8_to_wstring(text);
  FontManager_printText(tempWstr.c_str(), tempWstr.size(),
                        FontOptions(Vec2(x, y)));
}

void FontManager_printText(const std::wstring& text,
                           const FontOptions& options) {
  FontManager_printText(text.c_str(), text.length(), options);
}

void FontManager_printText(const std::wstring& text, const float& x,
                           const float& y) {
  FontManager_printText(text.c_str(), text.size(), FontOptions(Vec2(x, y)));
}