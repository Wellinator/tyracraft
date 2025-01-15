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

  const std::string Label_WelcomeText1Part1 = LanguageManager::Translate("/state_welcome/welcome_text_part1");
  const std::string Label_WelcomeText1Part2 = LanguageManager::Translate("/state_welcome/welcome_text_part2");
  const std::string Label_WelcomeText1Part3 = LanguageManager::Translate("/state_welcome/welcome_text_part3");
  const std::string Label_WelcomeText1Part4 = LanguageManager::Translate("/state_welcome/welcome_text_part4");
  const std::string Label_WelcomeText1Part5 = LanguageManager::Translate("/state_welcome/welcome_text_part5");
  const std::string Label_WelcomeText1Part6 = LanguageManager::Translate("/state_welcome/welcome_text_part6");
  const std::string Label_WelcomeText1Part7 = LanguageManager::Translate("/state_welcome/welcome_text_part7");

  void unloadTextures();
};
