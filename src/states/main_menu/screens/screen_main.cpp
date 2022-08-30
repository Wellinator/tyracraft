#pragma once
#include "states/main_menu/screens/screen_main.hpp"
#include "states/main_menu/screens/screen_how_to_play.hpp"

ScreenMain::ScreenMain(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = t_context->context->t_renderer;
  this->init();
}

ScreenMain::~ScreenMain() {
  this->t_renderer->getTextureRepository().freeBySprite(raw_slot[0]);
  this->t_renderer->getTextureRepository().freeBySprite(raw_slot[1]);
  this->t_renderer->getTextureRepository().freeBySprite(raw_slot[2]);
  this->t_renderer->getTextureRepository().freeBySprite(active_slot);

  this->t_renderer->getTextureRepository().freeBySprite(textPlayGame);
  this->t_renderer->getTextureRepository().freeBySprite(textHowToPlay);
  this->t_renderer->getTextureRepository().freeBySprite(textAbout);

  this->t_renderer->getTextureRepository().freeBySprite(btnCross);
  this->t_renderer->getTextureRepository().freeBySprite(textSelect);
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

  this->t_renderer->renderer2D.render(&textPlayGame);
  this->t_renderer->renderer2D.render(&textHowToPlay);
  this->t_renderer->renderer2D.render(&textAbout);

  this->t_renderer->renderer2D.render(&btnCross);
  this->t_renderer->renderer2D.render(&textSelect);
}

void ScreenMain::init() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = this->t_renderer->core.getSettings().getHeight() / 2;

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

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/slot.png"))
      ->addLink(raw_slot[0].id);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/slot.png"))
      ->addLink(raw_slot[1].id);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/slot.png"))
      ->addLink(raw_slot[2].id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 240);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/slot_active.png"))
      ->addLink(active_slot.id);

  // Texts
  textPlayGame.mode = Tyra::MODE_STRETCH;
  textPlayGame.size.set(128, 16);
  textPlayGame.position.set(halfWidth - 64, 240 + 9);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_play_game.png"))
      ->addLink(textPlayGame.id);

  textHowToPlay.mode = Tyra::MODE_STRETCH;
  textHowToPlay.size.set(128, 16);
  textHowToPlay.position.set(halfWidth - 64, 240 + 49);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_how_to_play.png"))
      ->addLink(textHowToPlay.id);

  textAbout.mode = Tyra::MODE_STRETCH;
  textAbout.size.set(64, 16);
  textAbout.position.set(halfWidth - 32, 240 + 89);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_about.png"))
      ->addLink(textAbout.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/textures/ui/btn_cross.png"))
      ->addLink(btnCross.id);

  textSelect.mode = Tyra::MODE_STRETCH;
  textSelect.size.set(64, 15);
  textSelect.position.set(
      15 + 30, this->t_renderer->core.getSettings().getHeight() - 36);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_select.png"))
      ->addLink(textSelect.id);
}

void ScreenMain::handleInput() {
  // Change active option
  {
    if (this->context->context->t_pad->getClicked().DpadDown) {
      int nextOption = (int)this->activeOption + 1;
      if (nextOption > 2)
        this->activeOption = ScreenMainOptions::PlayGame;
      else
        this->activeOption = static_cast<ScreenMainOptions>(nextOption);
      TYRA_LOG("New option -> ", (int)this->activeOption);
    } else if (this->context->context->t_pad->getClicked().DpadUp) {
      int nextOption = (int)this->activeOption - 1;
      if (nextOption < 0)
        this->activeOption = ScreenMainOptions::About;
      else
        this->activeOption = static_cast<ScreenMainOptions>(nextOption);
      TYRA_LOG("New option -> ", (int)this->activeOption);
    }
  }

  if (this->context->context->t_pad->getClicked().Cross)
    this->selectedOption = this->activeOption;
}

void ScreenMain::hightLightActiveOption() {
  // Reset sprite colors
  {
    this->textPlayGame.color = Tyra::Color(128, 128, 128);
    this->textHowToPlay.color = Tyra::Color(128, 128, 128);
    this->textAbout.color = Tyra::Color(128, 128, 128);
  }

  Sprite* t_selectedOptionSprite = NULL;
  if (this->activeOption == ScreenMainOptions::PlayGame)
    t_selectedOptionSprite = &this->textPlayGame;
  else if (this->activeOption == ScreenMainOptions::HowToPlay)
    t_selectedOptionSprite = &this->textHowToPlay;
  else
    t_selectedOptionSprite = &this->textAbout;

  t_selectedOptionSprite->color = Tyra::Color(255, 255, 0);

  // Hightlight active slot
  {
    u8 option = (int)this->activeOption;
    this->active_slot.position.y =
        (option * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  }
}

void ScreenMain::navigate() {
  if (this->selectedOption == ScreenMainOptions::None) return;

  // TODO: PlayGame wold options;
  if (this->selectedOption == ScreenMainOptions::PlayGame)
    this->context->loadGame();
  else if (this->selectedOption == ScreenMainOptions::HowToPlay)
    this->context->setScreen(new ScreenHowToPlay(this->context));

  // TODO: About screens;
}