#include "states/game_play/states/in_game_menu/state_game_menu.hpp"
#include "managers/font/font_manager.hpp"

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

  horizontalScrollArea.mode = Tyra::MODE_STRETCH;
  horizontalScrollArea.size.set(SLOT_WIDTH, 32);
  horizontalScrollArea.position.set(halfWidth - SLOT_WIDTH / 2, 215);
  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/horizontal_scroll_area.png"))
      ->addLink(horizontalScrollArea.id);

  horizontalScrollHandler.mode = Tyra::MODE_STRETCH;
  horizontalScrollHandler.size.set(16, 16);
  horizontalScrollHandler.position.set(halfWidth - SLOT_WIDTH / 2, 215);
  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/horizontal_scroll_handler.png"))
      ->addLink(horizontalScrollHandler.id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 240);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot_active.png"))
      ->addLink(active_slot.id);

  // Texts
  textGameMenu.mode = Tyra::MODE_STRETCH;
  textGameMenu.size.set(128, 16);
  textGameMenu.position.set(halfWidth - 64, halfHeight - 200);

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

  updateDrawDistanceScroll();
}

void StateGameMenu::update(const float& deltaTime) {
  this->handleInput(deltaTime);
  this->hightLightActiveOption();
  this->navigate();
}

void StateGameMenu::render() {
  // Render the game under menu
  stateGamePlay->getPreviousState()->render();

  t_renderer->renderer2D.render(overlay);

  FontOptions drawDistanceLabel;
  drawDistanceLabel.position.set(165, 180);
  if (activeOption == GameMenuOptions::DrawDistance)
    drawDistanceLabel.color.set(128, 128, 0);
  FontManager_printText("Draw Distance", drawDistanceLabel);
  t_renderer->renderer2D.render(horizontalScrollArea);
  t_renderer->renderer2D.render(horizontalScrollHandler);

  t_renderer->renderer2D.render(raw_slot[0]);
  t_renderer->renderer2D.render(raw_slot[1]);
  if (activeOption != GameMenuOptions::DrawDistance)
    t_renderer->renderer2D.render(active_slot);

  t_renderer->renderer2D.render(textGameMenu);
  t_renderer->renderer2D.render(textBackToGame);
  t_renderer->renderer2D.render(textQuitToTitle);

  t_renderer->renderer2D.render(btnCross);
  t_renderer->renderer2D.render(textSelect);
}

void StateGameMenu::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.DpadDown) {
    int nextOption = (int)this->activeOption + 1;
    if (nextOption > 2)
      this->activeOption = GameMenuOptions::DrawDistance;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);

  } else if (clicked.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = GameMenuOptions::QuitToTitle;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);
  }

  if (activeOption == GameMenuOptions::DrawDistance) {
    if (clicked.DpadLeft)
      decreaseDrawDistance();
    else if (clicked.DpadRight)
      increaseDrawDistance();
  }

  if (clicked.Cross) {
    this->playClickSound();
    this->selectedOption = this->activeOption;
  }
}

void StateGameMenu::navigate() {
  if (this->selectedOption == GameMenuOptions::None) {
    return;
  } else if (this->selectedOption == GameMenuOptions::BackToGame)
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
  textureRepository->freeBySprite(horizontalScrollArea);
  textureRepository->freeBySprite(horizontalScrollHandler);
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
  if (this->activeOption == GameMenuOptions::BackToGame) {
    t_selectedOptionSprite = &this->textBackToGame;
    this->active_slot.position.y =
        (0 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  } else if (this->activeOption == GameMenuOptions::QuitToTitle) {
    t_selectedOptionSprite = &this->textQuitToTitle;
    this->active_slot.position.y =
        (1 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  }

  if (t_selectedOptionSprite) {
    t_selectedOptionSprite->color = Tyra::Color(255, 255, 0);
  }
}

void StateGameMenu::increaseDrawDistance() {
  stateGamePlay->world->setDrawDistace(stateGamePlay->world->getDrawDistace() +
                                       1);
  updateDrawDistanceScroll();
}

void StateGameMenu::decreaseDrawDistance() {
  stateGamePlay->world->setDrawDistace(stateGamePlay->world->getDrawDistace() -
                                       1);
  updateDrawDistanceScroll();
}

void StateGameMenu::updateDrawDistanceScroll() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float min = halfWidth - (SLOT_WIDTH / 2);
  const float distance =
      stateGamePlay->world->getDrawDistace() - MIN_DRAW_DISTANCE;

  float porcentage = distance / (MAX_DRAW_DISTANCE - MIN_DRAW_DISTANCE);

  float value =
      (porcentage * SLOT_WIDTH) - (porcentage * horizontalScrollHandler.size.x);

  horizontalScrollHandler.position.x = min + value;
}
