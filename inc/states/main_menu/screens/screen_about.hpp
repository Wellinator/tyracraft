#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
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

  void handleInput();
  void navigate();
  void renderAboutText();
  void renderAboutText1();
  void renderAboutText2();
  void renderAboutText3();
  u8 canReturnToMainMenu();
};
