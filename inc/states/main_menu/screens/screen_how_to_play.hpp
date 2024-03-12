#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "states/main_menu/state_main_menu.hpp"
#include <tamtypes.h>
#include <tyra>

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;

class ScreenHowToPlay : public ScreenBase {
 public:
  ScreenHowToPlay(StateMainMenu* t_context);
  ~ScreenHowToPlay();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  Renderer* t_renderer;
  Sprite how_to_play;
  Sprite textBack;
  Sprite btnTriangle;

  void handleInput();
  void navigate();
};
