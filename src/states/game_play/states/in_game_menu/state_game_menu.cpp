#include "states/game_play/states/in_game_menu/state_game_menu.hpp"

StateGameMenu::StateGameMenu(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

StateGameMenu::~StateGameMenu() { this->unloadTextures(); }

void StateGameMenu::init() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = this->t_renderer->core.getSettings().getHeight() / 2;

  // Overlay
  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(halfWidth * 2, halfHeight * 2);
  overlay.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);

  // Load slots
  raw_slot[0].mode = Tyra::MODE_STRETCH;
  raw_slot[0].size.set(SLOT_WIDTH, 35);
  raw_slot[0].position.set(halfWidth - SLOT_WIDTH / 2, 240);
  raw_slot[1].mode = Tyra::MODE_STRETCH;
  raw_slot[1].size.set(SLOT_WIDTH, 35);
  raw_slot[1].position.set(halfWidth - SLOT_WIDTH / 2, 240 + 40);

  this->textureRawSlot = this->t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/gui/slot.png"));

  this->textureRawSlot->addLink(raw_slot[0].id);
  this->textureRawSlot->addLink(raw_slot[1].id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 240);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot_active.png"))
      ->addLink(active_slot.id);

  // Texts
  textGameMenu.mode = Tyra::MODE_STRETCH;
  textGameMenu.size.set(128, 16);
  textGameMenu.position.set(halfWidth - 64, halfHeight - 100);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/text_game_menu.png"))
      ->addLink(textGameMenu.id);

  textBackToGame.mode = Tyra::MODE_STRETCH;
  textBackToGame.size.set(128, 16);
  textBackToGame.position.set(halfWidth - 64, 240 + 9);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/text_back_to_game.png"))
      ->addLink(textBackToGame.id);

  textQuitToTitle.mode = Tyra::MODE_STRETCH;
  textQuitToTitle.size.set(128, 16);
  textQuitToTitle.position.set(halfWidth - 64, 240 + 49);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/text_quit_to_title.png"))
      ->addLink(textQuitToTitle.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  textSelect.mode = Tyra::MODE_STRETCH;
  textSelect.size.set(64, 15);
  textSelect.position.set(
      15 + 30, this->t_renderer->core.getSettings().getHeight() - 36);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/text_select.png"))
      ->addLink(textSelect.id);
}

void StateGameMenu::update(const float& deltaTime) {
  this->handleInput(deltaTime);
  Threading::switchThread();
  this->hightLightActiveOption();
  Threading::switchThread();
  this->navigate();
}

void StateGameMenu::render() {
  // Render the game under menu
  this->stateGamePlay->getPreviousState()->render();

  Threading::switchThread();
  this->t_renderer->renderer2D.render(&overlay);

  Threading::switchThread();
  this->t_renderer->renderer2D.render(&raw_slot[0]);
  this->t_renderer->renderer2D.render(&raw_slot[1]);
  this->t_renderer->renderer2D.render(&active_slot);

  Threading::switchThread();
  this->t_renderer->renderer2D.render(&textGameMenu);
  this->t_renderer->renderer2D.render(&textBackToGame);
  this->t_renderer->renderer2D.render(&textQuitToTitle);

  Threading::switchThread();
  this->t_renderer->renderer2D.render(&btnCross);
  this->t_renderer->renderer2D.render(&textSelect);
}

void StateGameMenu::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.DpadDown) {
    int nextOption = (int)this->activeOption + 1;
    if (nextOption > 1)
      this->activeOption = GameMenuOptions::BackToGame;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);

  } else if (clicked.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = GameMenuOptions::QuitToTitle;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);
  }

  if (clicked.Cross) {
    this->playClickSound();
    this->selectedOption = this->activeOption;
  }
}

void StateGameMenu::navigate() {
  if (this->selectedOption == GameMenuOptions::None) return;

  // TODO: PlayGame wold options;
  if (this->selectedOption == GameMenuOptions::BackToGame)
    this->stateGamePlay->backToGame();
  else if (this->selectedOption == GameMenuOptions::QuitToTitle)
    this->stateGamePlay->quitToTitle();
}

void StateGameMenu::unloadTextures() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->free(this->textureRawSlot->id);
  textureRepository->freeBySprite(overlay);
  textureRepository->freeBySprite(active_slot);
  textureRepository->freeBySprite(btnCross);
  textureRepository->freeBySprite(textSelect);
  textureRepository->freeBySprite(textGameMenu);
  textureRepository->freeBySprite(textBackToGame);
  textureRepository->freeBySprite(textQuitToTitle);
}

void StateGameMenu::playClickSound() {
  const s8 ch =
      this->stateGamePlay->context->t_soundManager->getAvailableChannel();
  this->stateGamePlay->context->t_engine->audio.adpcm.setVolume(60, ch);
  this->stateGamePlay->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                                        SoundFX::Click, ch);
}

void StateGameMenu::hightLightActiveOption() {
  // Reset sprite colors
  {
    this->textBackToGame.color = Tyra::Color(128, 128, 128);
    this->textQuitToTitle.color = Tyra::Color(128, 128, 128);
  }

  Sprite* t_selectedOptionSprite = nullptr;
  if (this->activeOption == GameMenuOptions::BackToGame)
    t_selectedOptionSprite = &this->textBackToGame;
  else if (this->activeOption == GameMenuOptions::QuitToTitle)
    t_selectedOptionSprite = &this->textQuitToTitle;

  if (t_selectedOptionSprite != nullptr) {
    t_selectedOptionSprite->color = Tyra::Color(255, 255, 0);
  }

  // Hightlight active slot
  {
    u8 option = (int)this->activeOption;
    this->active_slot.position.y =
        (option * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  }
}
