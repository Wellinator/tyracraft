#pragma once
#include "states/main_menu/screens/screen_how_to_play.hpp"
#include "states/main_menu/screens/screen_main.hpp"

ScreenHowToPlay::ScreenHowToPlay(StateMainMenu* t_context)
    : ScreenBase(t_context) {
  this->t_renderer = t_context->context->t_renderer;
  this->init();
}

ScreenHowToPlay::~ScreenHowToPlay() {
  this->t_renderer->getTextureRepository().freeBySprite(how_to_play);
  this->t_renderer->getTextureRepository().freeBySprite(btnTriangle);
  this->t_renderer->getTextureRepository().freeBySprite(textBack);
}

void ScreenHowToPlay::update() { this->handleInput(); }

void ScreenHowToPlay::render() {
  this->t_renderer->renderer2D.render(&how_to_play);
  this->t_renderer->renderer2D.render(&btnTriangle);
  this->t_renderer->renderer2D.render(&textBack);
}

void ScreenHowToPlay::init() {
  how_to_play.mode = Tyra::MODE_STRETCH;
  how_to_play.size.set(512, 512);
  how_to_play.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/how_to_play.png"))
      ->addLink(how_to_play.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(
      15, this->t_renderer->core.getSettings().getHeight() - 40);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/textures/ui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  textBack.mode = Tyra::MODE_STRETCH;
  textBack.size.set(64, 15);
  textBack.position.set(15 + 30,
                        this->t_renderer->core.getSettings().getHeight() - 36);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_back.png"))
      ->addLink(textBack.id);
}

void ScreenHowToPlay::handleInput() {
  if (this->context->context->t_pad->getClicked().Triangle) {
    this->playClickSound();
    this->navigate();
  };
}

void ScreenHowToPlay::navigate() {
  this->context->setScreen(new ScreenMain(this->context));
}

void ScreenHowToPlay::playClickSound() {
  this->context->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                                  SoundFX::Click);
}
