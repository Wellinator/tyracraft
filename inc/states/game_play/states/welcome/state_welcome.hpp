#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/language_manager.hpp"
#include "constants.hpp"
#include "entities/World.hpp"
#include <string>
#include <tamtypes.h>
#include <tyra>

using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::TextureRepository;

class StateWelcome : public PlayingStateBase {
 public:
  StateWelcome(StateGamePlay* t_context);
  ~StateWelcome();

  void init();
  void afterInit(){};
  void handleAction(MenuAction action){};
  void update(const float& deltaTime);
  void tick(){};
  void render();
  void handleInput(const float& deltaTime);

  void playClickSound();

 private:
  Renderer* t_renderer;

  Sprite overlay;
  Sprite btnCross;

  const std::string Label_WelcomeText1Part1 = g_language_repository["state_welcome"]["welcome_text_part1"].get<std::string>();
  const std::string Label_WelcomeText1Part2 = g_language_repository["state_welcome"]["welcome_text_part2"].get<std::string>();
  const std::string Label_WelcomeText1Part3 = g_language_repository["state_welcome"]["welcome_text_part3"].get<std::string>();
  const std::string Label_WelcomeText1Part4 = g_language_repository["state_welcome"]["welcome_text_part4"].get<std::string>();
  const std::string Label_WelcomeText1Part5 = g_language_repository["state_welcome"]["welcome_text_part5"].get<std::string>();
  const std::string Label_WelcomeText1Part6 = g_language_repository["state_welcome"]["welcome_text_part6"].get<std::string>();
  const std::string Label_WelcomeText1Part7 = g_language_repository["state_welcome"]["welcome_text_part7"].get<std::string>();

  void unloadTextures();
};
