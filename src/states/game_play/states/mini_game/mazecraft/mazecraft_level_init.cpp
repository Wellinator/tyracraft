#include "states/game_play/states/minigame/mazecraft/mazecraft_level_init.hpp"
#include "managers/font/font_manager.hpp"

MazecraftLevelInit::MazecraftLevelInit(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

MazecraftLevelInit::~MazecraftLevelInit() { this->unloadTextures(); }

void MazecraftLevelInit::init() {
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

void MazecraftLevelInit::update(const float& deltaTime) {
  this->handleInput(deltaTime);
}

void MazecraftLevelInit::render() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  stateGamePlay->getPreviousState()->render();
  t_renderer->renderer2D.render(overlay);

  FontOptions titleOption;
  titleOption.position.set(halfWidth - 20, 110);
  titleOption.alignment = TextAlignment::Center;
  titleOption.scale = 1.6F;
  FontManager_printText(
      Label_Level + std::string(" ") +
          std::to_string(stateGamePlay->world->getWorldOptions()->seed),
      titleOption);

  FontOptions textOption;
  textOption.position.set(halfWidth - 10, 185);
  textOption.alignment = TextAlignment::Center;
  textOption.scale = 0.8F;
  FontManager_printText(Label_WelcomeText1Part1, textOption);
  textOption.position.y += 20;
  FontManager_printText(Label_WelcomeText1Part2, textOption);
  textOption.position.y += 20;
  FontManager_printText(Label_WelcomeText1Part3, textOption);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(Label_WelcomeText1Part4, 40, 407);
}

void MazecraftLevelInit::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.Cross) {
    this->playClickSound();
    this->stateGamePlay->backToGame();
  }
}

void MazecraftLevelInit::unloadTextures() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->freeBySprite(overlay);
  textureRepository->freeBySprite(btnCross);
}

void MazecraftLevelInit::playClickSound() {
  const s8 ch =
      this->stateGamePlay->context->t_soundManager->getAvailableChannel();
  this->stateGamePlay->context->t_engine->audio.adpcm.setVolume(60, ch);
  this->stateGamePlay->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                                        SoundFX::WoodClick, ch);
}