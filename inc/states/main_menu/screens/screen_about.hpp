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
  void update();
  void render();

 private:
  Renderer* t_renderer;
  Sprite about_background;
  Sprite textBack;
  Sprite btnTriangle;
  u8 alpha = 1;
  u8 isFading = 0;
  u8 hasShowedText1 = 0;
  u8 hasShowedText2 = 0;
  u8 hasShowedText3 = 0;
  float BASE_WIDTH;

  const std::string Label_Text1Part1 = g_language_repository["screen_about"]["about_text1_part1"].get<std::string>();
  const std::string Label_Text1Part2 = g_language_repository["screen_about"]["about_text1_part2"].get<std::string>();
  const std::string Label_Text1Part3 = g_language_repository["screen_about"]["about_text1_part3"].get<std::string>();
  const std::string Label_Text1Part4 = g_language_repository["screen_about"]["about_text1_part4"].get<std::string>();
  const std::string Label_Text1Part5 = g_language_repository["screen_about"]["about_text1_part5"].get<std::string>();
  const std::string Label_Text1Part6 = g_language_repository["screen_about"]["about_text1_part6"].get<std::string>();
  const std::string Label_Text1Part7 = g_language_repository["screen_about"]["about_text1_part7"].get<std::string>();

  const std::string Label_Text2Part1 = g_language_repository["screen_about"]["about_text2_part1"].get<std::string>();
  const std::string Label_Text2Part2 = g_language_repository["screen_about"]["about_text2_part2"].get<std::string>();

  const std::string Label_Text3Part1 = g_language_repository["screen_about"]["about_text3_part1"].get<std::string>();
  const std::string Label_Text3Part2 = g_language_repository["screen_about"]["about_text3_part2"].get<std::string>();
  const std::string Label_Text3Part3 = g_language_repository["screen_about"]["about_text3_part3"].get<std::string>();
  const std::string Label_Text3Part4 = g_language_repository["screen_about"]["about_text3_part4"].get<std::string>();

  void handleInput();
  void navigate();
  void renderAboutText();
  void renderAboutText1();
  void renderAboutText2();
  void renderAboutText3();
  u8 canReturnToMainMenu();
};
