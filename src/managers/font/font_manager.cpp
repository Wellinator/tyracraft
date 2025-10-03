#include "managers/font/font_manager.hpp"

FontManager::FontManager(Renderer* t_renderer) : Singleton<FontManager>() {
  pRenderer = t_renderer;
  init();
}

FontManager::~FontManager() {}

void FontManager::init() { loadFontChars(); }

void FontManager::printText(const char* text, const FontOptions& options) {
  const std::wstring tempWstr = utf8_to_wstring(std::string(text));
  printText(tempWstr.c_str(), tempWstr.length(), options);
}

void FontManager::printText(const char* text, const float& x, const float& y) {
  const std::wstring tempWstr = utf8_to_wstring(std::string(text));
  printText(tempWstr.c_str(), tempWstr.length(), FontOptions(Vec2(x, y)));
}

void FontManager::printText(const wchar_t* text, const size_t length,
                            const FontOptions& options) {
  if (length == 0) return;

  const float padding = calcLinePadding(text, length, options.alignment) * options.scale;
  const float lineHeight = BASE_LINE_HEIGHT * options.scale;
  const float baseX = options.position.x - padding;
  const float baseY = options.position.y;
  
  float cursorX = 0.0F;
  float cursorY = 0.0F;

  for (size_t i = 0; i < length; i++) {
    const u8 charCode = static_cast<u8>(text[i]);
    
    // Break line
    if (charCode == LINE_FEED) {
      cursorY += lineHeight;
      cursorX = 0.0F;
      continue;
    }

    Sprite* const fontCharAt = printable_ascii_chars_sprites[charCode];

    if (fontCharAt != nullptr) {
      fontCharAt->position.set(baseX + cursorX, baseY + cursorY);
      fontCharAt->color = options.color;
      fontCharAt->scale = options.scale;

      pRenderer->renderer2D.render(*fontCharAt);
      cursorX += char_widths[charCode] * options.scale;
    }
  }
}

float FontManager::calcLinePadding(const wchar_t* text, const size_t length,
                                   const TextAlignment alignment) {
  if (alignment == TextAlignment::Left) return 0.0F;

  float padding = 0.0F;
  for (size_t i = 0; i < length; i++) {
    padding += char_widths[static_cast<u8>(text[i])];
  }

  return (alignment == TextAlignment::Center) ? padding * 0.5F : padding;
}

void FontManager::loadFontChars() {
  constexpr int MAX_COLS = 16;
  constexpr float CHAR_SIZE = 32.0F;
  constexpr size_t TOTAL_CHARS = 256;

  Font_ASCII_Texture = pRenderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/font/ascii.png"));

  for (size_t code = 0; code < TOTAL_CHARS; code++) {
    Sprite* charSprite = new Sprite();

    charSprite->mode = Tyra::MODE_REPEAT;
    charSprite->size.set(CHAR_SIZE, CHAR_SIZE);

    const u8 x_offset = code % MAX_COLS;
    const u8 y_offset = code / MAX_COLS;
    charSprite->offset.set(x_offset * CHAR_SIZE, y_offset * CHAR_SIZE);

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
  if (str.empty()) return std::wstring();
  
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  try {
    return converter.from_bytes(str);
  } catch (...) {
    return std::wstring();
  }
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