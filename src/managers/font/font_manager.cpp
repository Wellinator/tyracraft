#include "managers/font/font_manager.hpp"

FontManager::FontManager(Renderer* renderer) {
  this->t_renderer = renderer;
  this->loadFontChars();
}

FontManager::~FontManager() { this->unloadFontChars(); };

void FontManager::printText(const std::string& text,
                            const FontOptions& options) {
  float cursorX = 0.0F;
  float cursorY = 0.0F;

  for (size_t i = 0; i < text.size(); i++) {
    const u8 charCode = getCodeFromChar(text.at(i));
    const FontChar* fontCharAt = getFontChatByCode(charCode);

    if (fontCharAt != nullptr) {
      // Break line
      if (fontCharAt->_charCode == LINE_FEED) {
        cursorY += BASE_LINE_HEIGHT * options.scale;
        cursorX = 0;
        continue;
      }

      Sprite spriteToPrint = *fontCharAt->t_sprite;
      spriteToPrint.position.set(options.position.x + cursorX,
                                 options.position.y + cursorY);
      spriteToPrint.color.set(options.color);
      spriteToPrint.scale = options.scale;

      t_renderer->renderer2D.render(spriteToPrint);
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

void FontManager::loadFontChars() {
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

    FontChar* fontChar = new FontChar(code, charSprite);
    printable_ascii_chars_sprites[code] = fontChar;
  }
};

void FontManager::unloadFontChars() {
  for (size_t i = 0; i < 256; i++) {
    delete printable_ascii_chars_sprites[i];
  }
}

const FontChar* FontManager::getFontChatByCode(const u8& code) {
  return printable_ascii_chars_sprites[code];
}
