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

    // index 47 - 56 are number from 0 to 9;
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    //

    12, 4, 4, 10, 12, 10, 12,

    // index 64 - 89 are number upper case letter from A to Z;
    14, 12, 12, 12, 12, 12, 12, 12, 12, 8, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12,
    //

    12, 8, 12, 8, 12, 12,

    // index 64 - 89 are number lower case letter from a to z;
    6, 12, 12, 12, 12, 12, 10, 10, 10, 8, 12, 10, 6, 12, 12, 12, 10, 10, 12, 12,
    8, 12, 12, 12, 12, 12,
    //

    12, 10, 4, 10, 14, 4, 12, 0, 6, 12, 10, 12, 8, 8, 8, 16, 12, 6, 20, 0, 12,
    0, 0, 6, 6, 10, 10, 6, 6, 12, 12, 16, 12, 6, 20, 0, 12, 12, 3, 4, 10, 12,
    12, 12, 4, 12, 10, 16, 8, 10, 10, 4, 16, 12, 8, 12, 8, 8, 6, 12, 12, 4, 8,
    8, 8, 10, 18, 18, 18, 12, 12, 12, 12, 12, 12, 12, 20, 12, 12, 12, 12, 12, 8,
    8, 8, 8, 14, 12, 12, 12, 12, 12, 12, 10, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 20, 12, 12, 12, 12, 12, 4, 6, 6, 6, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};

void FontManager_init(Renderer* t_renderer) {
  g_t_renderer = t_renderer;
  FontManager_loadFontChars(t_renderer);
}

void FontManager_printText(const std::string& text, const FontOptions& options);

void FontManager_printText(const char* text, const FontOptions& options) {
  FontManager_printText(std::string(text), options);
}

void FontManager_printText(const char* text, const float& x, const float& y) {
  FontManager_printText(std::string(text), FontOptions(Vec2(x, y)));
}

void FontManager_printText(const std::string& text,
                           const FontOptions& options) {
  float cursorX = 0.0F;
  float cursorY = 0.0F;
  const u16 stringLenth = text.size();
  float padding = FontManager_calcLinePadding(text, options.alignment);
  padding *= options.scale;

  for (size_t i = 0; i < stringLenth; i++) {
    const u8 charCode = FontManager_getCodeFromChar(text.at(i));
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

float FontManager_calcLinePadding(const std::string& text,
                                  const TextAlignment alignment) {
  float padding = 0.0F;

  if (alignment == TextAlignment::Left)
    return padding;
  else {
    const size_t stringLenth = text.size();
    for (size_t i = 0; i < stringLenth; i++) {
      const u8 charCode = FontManager_getCodeFromChar(text.at(i));
      padding += (char_widths[charCode] + 2);
    }
  }

  return alignment == TextAlignment::Center ? padding / 2 : padding;
}

void FontManager_loadFontChars(Renderer* t_renderer) {
  const u8 INITIAL_CHAR_CODE = 0;
  const u8 FINAL_CHAR_CODE = 255;

  for (size_t code = INITIAL_CHAR_CODE; code <= FINAL_CHAR_CODE; code++) {
    Sprite* charSprite = new Sprite();
    charSprite->mode = Tyra::MODE_STRETCH;
    charSprite->size.set(32.0F, 32.0F);

    std::string charPath = "textures/font/chars/";
    charPath.append(std::to_string(code));
    charPath.append(".png");

    t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd(charPath))
        ->addLink(charSprite->id);

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
