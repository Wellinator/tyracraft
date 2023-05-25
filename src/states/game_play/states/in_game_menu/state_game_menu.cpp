#include "states/game_play/states/in_game_menu/state_game_menu.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/save_manager.hpp"

StateGameMenu::StateGameMenu(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

StateGameMenu::~StateGameMenu() { this->unloadTextures(); }

void StateGameMenu::init() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = this->t_renderer->core.getSettings().getHeight() / 2;

  // OverlayL
  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(halfWidth * 2, halfHeight * 2);
  overlay.position.set(0, 0);

  textureRepo->add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);

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

  this->textureRawSlot =
      textureRepo->add(FileUtils::fromCwd("textures/gui/slot.png"));

  this->textureRawSlot->addLink(raw_slot[0].id);
  this->textureRawSlot->addLink(raw_slot[1].id);
  this->textureRawSlot->addLink(raw_slot[2].id);

  horizontalScrollArea.mode = Tyra::MODE_STRETCH;
  horizontalScrollArea.size.set(SLOT_WIDTH, 32);
  horizontalScrollArea.position.set(halfWidth - SLOT_WIDTH / 2, 215);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/horizontal_scroll_area.png"))
      ->addLink(horizontalScrollArea.id);

  horizontalScrollHandler.mode = Tyra::MODE_STRETCH;
  horizontalScrollHandler.size.set(16, 16);
  horizontalScrollHandler.position.set(halfWidth - SLOT_WIDTH / 2, 215);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/horizontal_scroll_handler.png"))
      ->addLink(horizontalScrollHandler.id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 240);
  textureRepo->add(FileUtils::fromCwd("textures/gui/slot_active.png"))
      ->addLink(active_slot.id);

  dialogWindow.mode = Tyra::MODE_STRETCH;
  dialogWindow.size.set(260, 260);
  dialogWindow.position.set(halfWidth - 130, 120);
  textureRepo->add(FileUtils::fromCwd("textures/gui/window.png"))
      ->addLink(dialogWindow.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(185,
                           t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  btnStart.mode = Tyra::MODE_STRETCH;
  btnStart.size.set(25, 25);
  btnStart.position.set(185,
                        this->t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_start.png"))
      ->addLink(btnStart.id);

  updateDrawDistanceScroll();
}

void StateGameMenu::update(const float& deltaTime) {
  this->handleInput(deltaTime);
  this->hightLightActiveOption();
  this->navigate();
}

void StateGameMenu::render() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = this->t_renderer->core.getSettings().getHeight() / 2;
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
  t_renderer->renderer2D.render(raw_slot[2]);
  if (activeOption != GameMenuOptions::DrawDistance)
    t_renderer->renderer2D.render(active_slot);

  FontManager_printText("Game Menu", halfWidth - 64, halfHeight - 200);

  FontOptions saveGameLabel;
  saveGameLabel.position.set(246, 240 + 3);
  saveGameLabel.alignment = TextAlignment::Center;
  if (activeOption == GameMenuOptions::SaveGame)
    saveGameLabel.color.set(128, 128, 0);
  FontManager_printText("Save Game", saveGameLabel);

  FontOptions backToGameLabel;
  backToGameLabel.position.set(246, 240 + 43);
  backToGameLabel.alignment = TextAlignment::Center;
  if (activeOption == GameMenuOptions::SaveAndQuit)
    backToGameLabel.color.set(128, 128, 0);
  FontManager_printText("Save and Quit", backToGameLabel);

  FontOptions quitToTitleLabel;
  quitToTitleLabel.position.set(246, 240 + 83);
  quitToTitleLabel.alignment = TextAlignment::Center;
  if (activeOption == GameMenuOptions::QuitWithoutSave)
    quitToTitleLabel.color.set(128, 128, 0);
  FontManager_printText("Quit Without Save", quitToTitleLabel);

  if (needSaveOverwriteConfirmation) {
    renderSaveOverwritingDialog();
  } else if (needSaveAndQuitConfirmation) {
    renderSaveAndQuitDialog();
  } else if (needQuitWithoutSaveConfirmation) {
    renderQuitWithoutSavingDialog();
  } else {
    t_renderer->renderer2D.render(btnCross);
    FontManager_printText("Select", 40, 407);
    t_renderer->renderer2D.render(btnStart);
    FontManager_printText("Back to game", 205, 407);
  }
}

void StateGameMenu::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (needSaveOverwriteConfirmation) {
    if (clicked.Cross) {
      this->playClickSound();
      stateGamePlay->saveGame();
      needSaveOverwriteConfirmation = false;
    } else if (clicked.Triangle) {
      needSaveOverwriteConfirmation = false;
    }
    return;
  } else if (needSaveAndQuitConfirmation) {
    if (clicked.Cross) {
      playClickSound();
      stateGamePlay->saveGame();
      stateGamePlay->quitToTitle();
    } else if (clicked.Triangle) {
      needSaveAndQuitConfirmation = false;
    }
    return;
  } else if (needQuitWithoutSaveConfirmation) {
    if (clicked.Cross) {
      this->playClickSound();
      stateGamePlay->quitToTitle();
    } else if (clicked.Triangle) {
      needQuitWithoutSaveConfirmation = false;
    }
    return;
  }

  if (clicked.DpadDown) {
    int nextOption = (int)this->activeOption + 1;
    if (nextOption > (int)GameMenuOptions::QuitWithoutSave)
      this->activeOption = GameMenuOptions::DrawDistance;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);

  } else if (clicked.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = GameMenuOptions::QuitWithoutSave;
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
    if (activeOption == GameMenuOptions::SaveGame) {
      this->playClickSound();

      std::string saveFileName = FileUtils::fromCwd(
          "saves/" + this->stateGamePlay->world->getWorldOptions()->name +
          ".tcw");

      if (SaveManager::CheckIfSaveExist(saveFileName.c_str())) {
        needSaveOverwriteConfirmation = true;
      } else {
        stateGamePlay->saveGame();
      }
    } else if (activeOption == GameMenuOptions::SaveAndQuit) {
      this->playClickSound();
      needSaveAndQuitConfirmation = true;
    } else if (activeOption == GameMenuOptions::QuitWithoutSave) {
      this->playClickSound();
      needQuitWithoutSaveConfirmation = true;
    }

    this->selectedOption = this->activeOption;
  }
}

void StateGameMenu::navigate() {
  if (this->selectedOption == GameMenuOptions::None) {
    return;
  }
}

void StateGameMenu::unloadTextures() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->free(this->textureRawSlot->id);
  textureRepository->freeBySprite(overlay);
  textureRepository->freeBySprite(active_slot);
  textureRepository->freeBySprite(btnCross);
  textureRepository->freeBySprite(btnTriangle);
  textureRepository->freeBySprite(horizontalScrollArea);
  textureRepository->freeBySprite(horizontalScrollHandler);
  textureRepository->freeBySprite(dialogWindow);
  textureRepository->freeBySprite(btnStart);
}

void StateGameMenu::playClickSound() {
  const s8 ch =
      this->stateGamePlay->context->t_soundManager->getAvailableChannel();
  this->stateGamePlay->context->t_engine->audio.adpcm.setVolume(60, ch);
  this->stateGamePlay->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                                        SoundFX::WoodClick, ch);
}

void StateGameMenu::hightLightActiveOption() {
  Sprite* t_selectedOptionSprite = nullptr;
  if (this->activeOption == GameMenuOptions::SaveGame) {
    this->active_slot.position.y =
        (0 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  } else if (this->activeOption == GameMenuOptions::SaveAndQuit) {
    this->active_slot.position.y =
        (1 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  } else if (this->activeOption == GameMenuOptions::QuitWithoutSave) {
    this->active_slot.position.y =
        (2 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
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

void StateGameMenu::renderSaveOverwritingDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  FontManager_printText("Overwrite Save Game?", titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 190);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  FontManager_printText("A local save will be overwritten.", dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText("Do you want to continue?", dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Overwrite", 40, 407);

  t_renderer->renderer2D.render(btnTriangle);
  FontManager_printText("Cancel", 205, 407);
}

void StateGameMenu::renderSaveAndQuitDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  FontManager_printText("Save and Quit?", titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 180);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  FontManager_printText("Any previous save", dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText(" will be overwritten", dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText("Do you want to continue?", dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Save and quit", 40, 407);

  btnTriangle.position.x = 225;
  t_renderer->renderer2D.render(btnTriangle);
  FontManager_printText("Cancel", 245, 407);
  btnTriangle.position.x = 185;
}

void StateGameMenu::renderQuitWithoutSavingDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  FontManager_printText("Are you sure?", titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 190);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  FontManager_printText("All unsaved progress will be lost", dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText("Do you want to continue?", dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Quit", 40, 407);

  t_renderer->renderer2D.render(btnTriangle);
  FontManager_printText("Cancel", 205, 407);
}
