#include "states/main_menu/screens/screen_main.hpp"
#include "states/main_menu/screens/screen_how_to_play.hpp"
#include "states/main_menu/screens/screen_about.hpp"
#include "states/main_menu/screens/screen_new_game.hpp"
#include "managers/font/font_manager.hpp"

ScreenMain::ScreenMain(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

ScreenMain::~ScreenMain() {
  this->t_renderer->getTextureRepository().freeBySprite(raw_slot[0]);
  this->t_renderer->getTextureRepository().freeBySprite(raw_slot[1]);
  this->t_renderer->getTextureRepository().freeBySprite(raw_slot[2]);
  this->t_renderer->getTextureRepository().freeBySprite(active_slot);

  this->t_renderer->getTextureRepository().freeBySprite(btnCross);
}

void ScreenMain::update() {
  this->handleInput();
  this->hightLightActiveOption();
  this->navigate();
}

void ScreenMain::render() {
  this->t_renderer->renderer2D.render(&raw_slot[0]);
  this->t_renderer->renderer2D.render(&raw_slot[1]);
  this->t_renderer->renderer2D.render(&raw_slot[2]);
  this->t_renderer->renderer2D.render(&active_slot);

  // Play Game
  {
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(256 - 64, 240 + 6));
    fontOptions.color.set(this->activeOption == ScreenMainOptions::PlayGame
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    FontManager_printText("Play Game", fontOptions);
  }

  // How To Play
  {
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(256 - 72, 240 + 46));
    fontOptions.color.set(this->activeOption == ScreenMainOptions::HowToPlay
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));
    FontManager_printText("How to Play", fontOptions);
  }

  // About
  {
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(256 - 42, 240 + 86));
    fontOptions.color.set(this->activeOption == ScreenMainOptions::About
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));
    FontManager_printText("About", fontOptions);
  }

  this->t_renderer->renderer2D.render(&btnCross);
  FontManager_printText("Select", 35, 407);
}

void ScreenMain::init() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;

  // Load slots
  raw_slot[0].mode = Tyra::MODE_STRETCH;
  raw_slot[0].size.set(SLOT_WIDTH, 35);
  raw_slot[0].position.set(halfWidth - SLOT_WIDTH / 2, 240);
  raw_slot[1].mode = Tyra::MODE_STRETCH;
  raw_slot[1].size.set(SLOT_WIDTH, 35);
  raw_slot[1].position.set(halfWidth - SLOT_WIDTH / 2, 240 + 40);
  raw_slot[2].mode = Tyra::MODE_STRETCH;
  raw_slot[2].size.set(SLOT_WIDTH, 35);
  raw_slot[2].position.set(halfWidth - SLOT_WIDTH / 2, 240 + 80);

  // TODO: Refactor to use the same texture, use texture link
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot.png"))
      ->addLink(raw_slot[0].id);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot.png"))
      ->addLink(raw_slot[1].id);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot.png"))
      ->addLink(raw_slot[2].id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 240);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot_active.png"))
      ->addLink(active_slot.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);
}

void ScreenMain::handleInput() {
  // Change active option
  {
    if (this->context->context->t_engine->pad.getClicked().DpadDown) {
      int nextOption = (int)this->activeOption + 1;
      if (nextOption > 2)
        this->activeOption = ScreenMainOptions::PlayGame;
      else
        this->activeOption = static_cast<ScreenMainOptions>(nextOption);
    } else if (this->context->context->t_engine->pad.getClicked().DpadUp) {
      int nextOption = (int)this->activeOption - 1;
      if (nextOption < 0)
        this->activeOption = ScreenMainOptions::About;
      else
        this->activeOption = static_cast<ScreenMainOptions>(nextOption);
    }
  }

  if (this->context->context->t_engine->pad.getClicked().Cross) {
    this->context->playClickSound();
    this->selectedOption = this->activeOption;
  }
}

void ScreenMain::hightLightActiveOption() {
  u8 option = (int)this->activeOption;
  this->active_slot.position.y =
      (option * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
}

void ScreenMain::navigate() {
  if (this->selectedOption == ScreenMainOptions::None) return;

  // TODO: PlayGame wold options;
  if (this->selectedOption == ScreenMainOptions::PlayGame)
    this->context->setScreen(new ScreenNewGame(this->context));
  else if (this->selectedOption == ScreenMainOptions::HowToPlay)
    this->context->setScreen(new ScreenHowToPlay(this->context));
  else if (this->selectedOption == ScreenMainOptions::About)
    this->context->setScreen(new ScreenAbout(this->context));
}
