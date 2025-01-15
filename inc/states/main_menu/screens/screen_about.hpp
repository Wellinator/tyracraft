#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include "managers/language_manager.hpp"
#include <tamtypes.h>
#include <tyra>

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;

class ScreenAbout : public ScreenBase {
 public:
  ScreenAbout(StateMainMenu* t_context);
  ~ScreenAbout();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  Renderer* t_renderer;
  FontManager* pFontManager;
  Sprite about_background;
  Sprite textBack;
  Sprite btnTriangle;
  u8 alpha = 1;
  u8 isFading = 0;
  u8 hasShowedText1 = 0;
  u8 hasShowedText2 = 0;
  u8 hasShowedText3 = 0;
  float BASE_WIDTH;

  const std::string Label_Text1Part1 = LanguageManager::Translate("/screen_about/about_text1_part1");
  const std::string Label_Text1Part2 = LanguageManager::Translate("/screen_about/about_text1_part2");
  const std::string Label_Text1Part3 = LanguageManager::Translate("/screen_about/about_text1_part3");
  const std::string Label_Text1Part4 = LanguageManager::Translate("/screen_about/about_text1_part4");
  const std::string Label_Text1Part5 = LanguageManager::Translate("/screen_about/about_text1_part5");
  const std::string Label_Text1Part6 = LanguageManager::Translate("/screen_about/about_text1_part6");
  const std::string Label_Text1Part7 = LanguageManager::Translate("/screen_about/about_text1_part7");

  const std::string Label_Text2Part1 = LanguageManager::Translate("/screen_about/about_text2_part1");
  const std::string Label_Text2Part2 = LanguageManager::Translate("/screen_about/about_text2_part2");

  const std::string Label_Text3Part1 = LanguageManager::Translate("/screen_about/about_text3_part1");
  const std::string Label_Text3Part2 = LanguageManager::Translate("/screen_about/about_text3_part2");
  const std::string Label_Text3Part3 = LanguageManager::Translate("/screen_about/about_text3_part3");
  const std::string Label_Text3Part4 = LanguageManager::Translate("/screen_about/about_text3_part4");

  void handleInput();
  void navigate();
  void renderAboutText();
  void renderAboutText1();
  void renderAboutText2();
  void renderAboutText3();
  u8 canReturnToMainMenu();
};
