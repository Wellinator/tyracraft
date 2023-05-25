#include "states/game_play/states/welcome/state_welcome.hpp"
#include "managers/font/font_manager.hpp"

StateWelcome::StateWelcome(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

StateWelcome::~StateWelcome() { this->unloadTextures(); }

void StateWelcome::init() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = this->t_renderer->core.getSettings().getHeight() / 2;

  // Overlay
  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(halfWidth * 2, halfHeight * 2);
  overlay.position.set(0, 0);

  textureRepo->add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);
}

void StateWelcome::update(const float& deltaTime) {
  this->handleInput(deltaTime);
}

void StateWelcome::render() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  stateGamePlay->getPreviousState()->render();
  t_renderer->renderer2D.render(overlay);

  FontOptions titleOption;
  titleOption.position.set(halfWidth - 20, 110);
  titleOption.alignment = TextAlignment::Center;
  titleOption.scale = 1.8F;
  FontManager_printText("Welcome to TyraCraft!", titleOption);

  FontOptions textOption;
  textOption.position.set(halfWidth - 10, 185);
  textOption.alignment = TextAlignment::Center;
  textOption.scale = 0.8F;
  FontManager_printText("Hey guys! A message from Wellinator!", textOption);
  textOption.position.y += 20;
  FontManager_printText("Keep in mind that this is a pre-alpha release",
                        textOption);
  textOption.position.y += 20;
  FontManager_printText("it will surely contain many bugs and weird behaviour.",
                        textOption);
  textOption.position.y += 20;
  FontManager_printText("We hope you enjoy it!", textOption);
  textOption.position.y += 20;
  FontManager_printText("Stay tunned for new updates!", textOption);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Let's play!", 40, 407);
}

void StateWelcome::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.Cross) {
    this->playClickSound();
    this->stateGamePlay->backToGame();
  }
}

void StateWelcome::unloadTextures() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->freeBySprite(overlay);
  textureRepository->freeBySprite(btnCross);
}

void StateWelcome::playClickSound() {
  const s8 ch =
      this->stateGamePlay->context->t_soundManager->getAvailableChannel();
  this->stateGamePlay->context->t_engine->audio.adpcm.setVolume(60, ch);
  this->stateGamePlay->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                                        SoundFX::WoodClick, ch);
}