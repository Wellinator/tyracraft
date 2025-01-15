#include "managers/font/font_manager.hpp"

FontManager::FontManager(Renderer* t_renderer) : Singleton<FontManager>() {
  pRenderer = t_renderer;
  init();
}

FontManager::~FontManager() {}

void FontManager::init() { loadFontChars(); }

void FontManager::printText(const char* text, const FontOptions& options) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  std::wstring tempWstr = converter.from_bytes(text, text + strlen(text));
  printText(tempWstr.c_str(), tempWstr.length(), options);
}

void FontManager::printText(const char* text, const float& x, const float& y) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  std::wstring tempWstr = converter.from_bytes(text, text + strlen(text));
  printText(tempWstr.c_str(), tempWstr.length(), FontOptions(Vec2(x, y)));
}

void FontManager::printText(const wchar_t* text, const size_t length,
                            const FontOptions& options) {
  float cursorX = 0.0F;
  float cursorY = 0.0F;
  float padding = calcLinePadding(text, length, options.alignment);
  padding *= options.scale;

  for (size_t i = 0; i < length; i++) {
    const u8 charCode = u8(text[i]);
    Sprite* fontCharAt = printable_ascii_chars_sprites[charCode];

    if (fontCharAt != nullptr) {
      // Break line
      if (charCode == LINE_FEED) {
        cursorY += BASE_LINE_HEIGHT * options.scale;
        cursorX = 0;
        continue;
      }

      fontCharAt->position.set(options.position.x + cursorX - padding,
                               options.position.y + cursorY);
      fontCharAt->color.set(options.color);
      fontCharAt->scale = options.scale;

      pRenderer->renderer2D.render(*fontCharAt);
      cursorX += char_widths[charCode] * options.scale;
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

float FontManager::calcLinePadding(const wchar_t* text, const size_t length,
                                   const TextAlignment alignment) {
  if (alignment == TextAlignment::Left) return 0.0F;

  float padding = 0.0F;
  const size_t stringLenth = length;
  for (size_t i = 0; i < stringLenth; i++) {
    const u8 charCode = u8(text[i]);
    padding += char_widths[charCode];
  }

  return alignment == TextAlignment::Center ? padding / 2 : padding;
}

void FontManager::loadFontChars() {
  const int MAX_COLS = 16;
  const u8 INITIAL_CHAR_CODE = 0;
  const u8 FINAL_CHAR_CODE = 255;

  Font_ASCII_Texture = pRenderer->getTextureRepository().add(
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

void FontManager::unloadFontChars() {
  for (size_t i = 0; i < 256; i++) {
    delete printable_ascii_chars_sprites[i];
  }
}

// UTF-8
std::wstring FontManager::utf8_to_wstring(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.from_bytes(str);
}

size_t FontManager::utf8len(char* s) {
  size_t len = 0;
  for (; *s; ++s)
    if ((*s & 0xC0) != 0x80) ++len;
  return len;
}

void FontManager::printText(const std::string& text,
                            const FontOptions& options) {
  std::wstring tempWstr = utf8_to_wstring(text);
  printText(tempWstr.c_str(), tempWstr.length(), options);
};

void FontManager::printText(const std::string& text, const float& x,
                            const float& y) {
  std::wstring tempWstr = utf8_to_wstring(text);
  printText(tempWstr.c_str(), tempWstr.size(), FontOptions(Vec2(x, y)));
}

void FontManager::printText(const std::wstring& text,
                            const FontOptions& options) {
  printText(text.c_str(), text.length(), options);
}

void FontManager::printText(const std::wstring& text, const float& x,
                            const float& y) {
  printText(text.c_str(), text.size(), FontOptions(Vec2(x, y)));
}