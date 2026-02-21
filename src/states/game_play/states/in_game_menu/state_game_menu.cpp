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
  const float screenWidth = this->t_renderer->core.getSettings().getWidth();
  const float screenHeight = this->t_renderer->core.getSettings().getHeight();

  // Background
  background.mode = Tyra::MODE_STRETCH;
  background.size.set(512, 512);
  background.position.set(0, 0);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/menu/load_game_background.png"))
      ->addLink(background.id);

  // Overlay
  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(halfWidth * 2, halfHeight * 2);
  overlay.position.set(0, 0);

  textureRepo->add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);

  // Row 1: Two half-width slots side by side
  const float gridTotalWidth = SLOT_HALF_WIDTH * 2 + SLOT_GAP;
  const float gridStartX = halfWidth - gridTotalWidth / 2;

  raw_slot_half[0].mode = Tyra::MODE_STRETCH;
  raw_slot_half[0].size.set(SLOT_HALF_WIDTH, SLOT_HEIGHT);
  raw_slot_half[0].position.set(gridStartX, GRID_START_Y);

  raw_slot_half[1].mode = Tyra::MODE_STRETCH;
  raw_slot_half[1].size.set(SLOT_HALF_WIDTH, SLOT_HEIGHT);
  raw_slot_half[1].position.set(gridStartX + SLOT_HALF_WIDTH + SLOT_GAP,
                                GRID_START_Y);

  // Row 2: Save (full-width centered)
  raw_slot_full[0].mode = Tyra::MODE_STRETCH;
  raw_slot_full[0].size.set(SLOT_FULL_WIDTH, SLOT_HEIGHT);
  raw_slot_full[0].position.set(halfWidth - SLOT_FULL_WIDTH / 2,
                                GRID_START_Y + ROW_SPACING);

  // Row 3: Back to Game (full-width centered)
  raw_slot_full[1].mode = Tyra::MODE_STRETCH;
  raw_slot_full[1].size.set(SLOT_FULL_WIDTH, SLOT_HEIGHT);
  raw_slot_full[1].position.set(halfWidth - SLOT_FULL_WIDTH / 2,
                                GRID_START_Y + ROW_SPACING * 2);

  this->textureRawSlot =
      textureRepo->add(FileUtils::fromCwd("textures/gui/slot.png"));

  this->textureRawSlot->addLink(raw_slot_half[0].id);
  this->textureRawSlot->addLink(raw_slot_half[1].id);
  this->textureRawSlot->addLink(raw_slot_full[0].id);
  this->textureRawSlot->addLink(raw_slot_full[1].id);

  // Quit button in the bottom-right corner
  raw_slot_quit.mode = Tyra::MODE_STRETCH;
  raw_slot_quit.size.set(SLOT_QUIT_WIDTH, SLOT_HEIGHT);
  raw_slot_quit.position.set(screenWidth - SLOT_QUIT_WIDTH - 10,
                             screenHeight - SLOT_HEIGHT - 10);

  this->textureRawSlotQuit =
      textureRepo->add(FileUtils::fromCwd("textures/gui/slot.png"));
  this->textureRawSlotQuit->addLink(raw_slot_quit.id);

  // Active slot (will be resized dynamically)
  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_HALF_WIDTH, SLOT_HEIGHT);
  active_slot.position.set(gridStartX, GRID_START_Y);
  textureRepo->add(FileUtils::fromCwd("textures/gui/slot_active.png"))
      ->addLink(active_slot.id);

  // Dialog window
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
}

void StateGameMenu::update(const float& deltaTime) {
  this->handleInput(deltaTime);
  this->hightLightActiveOption();
  this->navigate();
}

void StateGameMenu::render() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  FontManager& fm = FontManager::getInstanceRef();

  t_renderer->renderer2D.render(background);

  // Title: "Menu do Jogo"
  fm.printText(Label_GameMenu, halfWidth - 64, GRID_START_Y - 40);

  // Row 1: Draw Distance (left) and FPS Mode (right)
  t_renderer->renderer2D.render(raw_slot_half[0]);
  t_renderer->renderer2D.render(raw_slot_half[1]);

  // Row 2-3: Save and Back to Game
  t_renderer->renderer2D.render(raw_slot_full[0]);
  t_renderer->renderer2D.render(raw_slot_full[1]);

  // Quit button in corner
  t_renderer->renderer2D.render(raw_slot_quit);

  // Active slot highlight
  t_renderer->renderer2D.render(active_slot);

  // Draw Distance label (left slot, row 1)
  {
    FontOptions fontOpts;
    fontOpts.position.set(raw_slot_half[0].position.x + SLOT_HALF_WIDTH / 2,
                          GRID_START_Y + 8);
    fontOpts.alignment = TextAlignment::Center;
    fontOpts.scale = 0.7F;
    if (activeOption == GameMenuOptions::DrawDistance)
      fontOpts.color.set(128, 128, 0);
    fm.printText(Label_DrawDistance + ": " + getDrawDistanceModeLabel(),
                 fontOpts);
  }

  // FPS Mode label (right slot, row 1)
  {
    FontOptions fontOpts;
    fontOpts.position.set(raw_slot_half[1].position.x + SLOT_HALF_WIDTH / 2,
                          GRID_START_Y + 8);
    fontOpts.alignment = TextAlignment::Center;
    fontOpts.scale = 0.7F;
    if (activeOption == GameMenuOptions::FpsModeOption)
      fontOpts.color.set(128, 128, 0);
    fm.printText(Label_FpsMode + ": " + getFpsModeLabel(), fontOpts);
  }

  // Save label (full-width, row 2)
  {
    FontOptions fontOpts;
    fontOpts.position.set(halfWidth, GRID_START_Y + ROW_SPACING + 3);
    fontOpts.alignment = TextAlignment::Center;
    if (activeOption == GameMenuOptions::SaveGame)
      fontOpts.color.set(128, 128, 0);
    fm.printText(Label_Save, fontOpts);
  }

  // Back to Game label (full-width, row 3)
  {
    FontOptions fontOpts;
    fontOpts.position.set(halfWidth, GRID_START_Y + ROW_SPACING * 2 + 3);
    fontOpts.alignment = TextAlignment::Center;
    if (activeOption == GameMenuOptions::BackToGame)
      fontOpts.color.set(128, 128, 0);
    fm.printText(Label_BackToGame, fontOpts);
  }

  // Quit label (corner button)
  {
    const float screenWidth = this->t_renderer->core.getSettings().getWidth();
    const float screenHeight = this->t_renderer->core.getSettings().getHeight();
    FontOptions fontOpts;
    fontOpts.position.set(screenWidth - SLOT_QUIT_WIDTH / 2 - 10,
                          screenHeight - SLOT_HEIGHT - 10 + 3);
    fontOpts.alignment = TextAlignment::Center;
    fontOpts.scale = 0.8F;
    if (activeOption == GameMenuOptions::Quit)
      fontOpts.color.set(128, 128, 0);
    fm.printText(Label_Quit, fontOpts);
  }

  if (needSaveOverwriteConfirmation) {
    renderSaveOverwritingDialog();
  } else if (needQuitConfirmation) {
    renderQuitWithoutSavingDialog();
  } else {
    t_renderer->renderer2D.render(btnCross);
    fm.printText(Label_Select, 40, 407);
  }
}

void StateGameMenu::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (needSaveOverwriteConfirmation) {
    if (clicked.Cross) {
      this->playClickSound();
      const auto oldMode = stateGamePlay->world->getDrawDistanceMode();
      stateGamePlay->world->setDrawDistanceMode(DrawDistanceMode::Low);

      stateGamePlay->saveGame();
      needSaveOverwriteConfirmation = false;

      stateGamePlay->world->setDrawDistanceMode(oldMode);
    } else if (clicked.Triangle) {
      needSaveOverwriteConfirmation = false;
    }
    return;
  } else if (needQuitConfirmation) {
    if (clicked.Cross) {
      this->playClickSound();
      stateGamePlay->quitToTitle();
    } else if (clicked.Triangle) {
      needQuitConfirmation = false;
    }
    return;
  }

  // 2D Grid navigation
  if (clicked.DpadDown) {
    switch (activeOption) {
      case GameMenuOptions::DrawDistance:
      case GameMenuOptions::FpsModeOption:
        activeOption = GameMenuOptions::SaveGame;
        break;
      case GameMenuOptions::SaveGame:
        activeOption = GameMenuOptions::BackToGame;
        break;
      case GameMenuOptions::BackToGame:
        activeOption = GameMenuOptions::Quit;
        break;
      case GameMenuOptions::Quit:
        activeOption = GameMenuOptions::DrawDistance;
        break;
      default:
        break;
    }
  } else if (clicked.DpadUp) {
    switch (activeOption) {
      case GameMenuOptions::DrawDistance:
      case GameMenuOptions::FpsModeOption:
        activeOption = GameMenuOptions::Quit;
        break;
      case GameMenuOptions::SaveGame:
        activeOption = GameMenuOptions::DrawDistance;
        break;
      case GameMenuOptions::BackToGame:
        activeOption = GameMenuOptions::SaveGame;
        break;
      case GameMenuOptions::Quit:
        activeOption = GameMenuOptions::BackToGame;
        break;
      default:
        break;
    }
  } else if (clicked.DpadLeft) {
    if (activeOption == GameMenuOptions::FpsModeOption)
      activeOption = GameMenuOptions::DrawDistance;
  } else if (clicked.DpadRight) {
    if (activeOption == GameMenuOptions::DrawDistance)
      activeOption = GameMenuOptions::FpsModeOption;
  }

  if (clicked.Cross) {
    this->playClickSound();
    if (activeOption == GameMenuOptions::DrawDistance) {
      cycleDrawDistanceMode(1);
    } else if (activeOption == GameMenuOptions::FpsModeOption) {
      cycleFpsMode(1);
    } else if (activeOption == GameMenuOptions::SaveGame) {
      std::string saveFileName = FileUtils::fromCwd(
          "saves/" + this->stateGamePlay->world->getWorldOptions()->name +
          ".tcw");

      if (SaveManager::CheckIfSaveExist(saveFileName.c_str())) {
        needSaveOverwriteConfirmation = true;
      } else {
        stateGamePlay->saveGame();
      }
    } else if (activeOption == GameMenuOptions::BackToGame) {
      stateGamePlay->backToGame();
      return;
    } else if (activeOption == GameMenuOptions::Quit) {
      needQuitConfirmation = true;
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
  textureRepository->free(this->textureRawSlotQuit->id);
  textureRepository->freeBySprite(background);
  textureRepository->freeBySprite(overlay);
  textureRepository->freeBySprite(active_slot);
  textureRepository->freeBySprite(btnCross);
  textureRepository->freeBySprite(btnTriangle);
  textureRepository->freeBySprite(dialogWindow);
}

void StateGameMenu::playClickSound() {
  SoundManager* pSoundManager = SoundManager::getInstance();

  const s8 ch = pSoundManager->getAvailableChannel();
  this->stateGamePlay->context->t_engine->audio.adpcm.setVolume(60, ch);
  pSoundManager->playSfx(SoundFxCategory::Random, SoundFX::WoodClick, ch);
}

void StateGameMenu::hightLightActiveOption() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float gridTotalWidth = SLOT_HALF_WIDTH * 2 + SLOT_GAP;
  const float gridStartX = halfWidth - gridTotalWidth / 2;
  const float screenWidth = this->t_renderer->core.getSettings().getWidth();
  const float screenHeight = this->t_renderer->core.getSettings().getHeight();

  switch (activeOption) {
    case GameMenuOptions::DrawDistance:
      active_slot.size.set(SLOT_HALF_WIDTH, SLOT_HEIGHT);
      active_slot.position.set(gridStartX, GRID_START_Y);
      break;
    case GameMenuOptions::FpsModeOption:
      active_slot.size.set(SLOT_HALF_WIDTH, SLOT_HEIGHT);
      active_slot.position.set(gridStartX + SLOT_HALF_WIDTH + SLOT_GAP,
                               GRID_START_Y);
      break;
    case GameMenuOptions::SaveGame:
      active_slot.size.set(SLOT_FULL_WIDTH, SLOT_HEIGHT);
      active_slot.position.set(halfWidth - SLOT_FULL_WIDTH / 2,
                               GRID_START_Y + ROW_SPACING);
      break;
    case GameMenuOptions::BackToGame:
      active_slot.size.set(SLOT_FULL_WIDTH, SLOT_HEIGHT);
      active_slot.position.set(halfWidth - SLOT_FULL_WIDTH / 2,
                               GRID_START_Y + ROW_SPACING * 2);
      break;
    case GameMenuOptions::Quit:
      active_slot.size.set(SLOT_QUIT_WIDTH, SLOT_HEIGHT);
      active_slot.position.set(screenWidth - SLOT_QUIT_WIDTH - 10,
                               screenHeight - SLOT_HEIGHT - 10);
      break;
    default:
      break;
  }
}

void StateGameMenu::cycleDrawDistanceMode(int direction) {
  int current = static_cast<int>(stateGamePlay->world->getDrawDistanceMode());
  current += direction;
  if (current > static_cast<int>(DrawDistanceMode::High))
    current = static_cast<int>(DrawDistanceMode::Auto);
  else if (current < static_cast<int>(DrawDistanceMode::Auto))
    current = static_cast<int>(DrawDistanceMode::High);
  stateGamePlay->world->setDrawDistanceMode(
      static_cast<DrawDistanceMode>(current));
}

void StateGameMenu::cycleFpsMode(int direction) {
  int current = static_cast<int>(g_settings.fps_mode);
  current += direction;
  if (current > static_cast<int>(FpsMode::FPS_60))
    current = static_cast<int>(FpsMode::VSync);
  else if (current < static_cast<int>(FpsMode::VSync))
    current = static_cast<int>(FpsMode::FPS_60);
  g_settings.fps_mode = static_cast<FpsMode>(current);
  // Apply immediately
  TyraCraft::Timer::getInstance()->setFpsMode(g_settings.fps_mode);
  SettingsManager::Save();
}

const std::string& StateGameMenu::getFpsModeLabel() const {
  switch (g_settings.fps_mode) {
    case FpsMode::VSync:  return Label_FpsVSync;
    case FpsMode::FPS_30: return Label_Fps30;
    default:              return Label_Fps60;
  }
}

const std::string& StateGameMenu::getDrawDistanceModeLabel() const {
  switch (stateGamePlay->world->getDrawDistanceMode()) {
    case DrawDistanceMode::Low:
      return Label_ModeLow;
    case DrawDistanceMode::Medium:
      return Label_ModeMedium;
    case DrawDistanceMode::High:
      return Label_ModeHigh;
    default:
      return Label_ModeAuto;
  }
}

void StateGameMenu::renderSaveOverwritingDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);
  FontManager& fm = FontManager::getInstanceRef();

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  fm.printText(Label_OverwriteGameAsk, titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 190);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  fm.printText(Label_LocalSaveWillBeOverWriten, dialogueOptions);
  dialogueOptions.position.y += 15;
  fm.printText(Label_DoYouWantToContinue, dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  fm.printText(Label_Overwtire, 40, 407);

  t_renderer->renderer2D.render(btnTriangle);
  fm.printText(Label_Cancel, 205, 407);
}

void StateGameMenu::renderQuitWithoutSavingDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);
  FontManager& fm = FontManager::getInstanceRef();

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  fm.printText(Label_AreYouSure, titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 190);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  fm.printText(Label_AllUnsavedProgressWillBeLost, dialogueOptions);
  dialogueOptions.position.y += 15;
  fm.printText(Label_DoYouWantToContinue, dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  fm.printText(Label_Quit, 40, 407);

  t_renderer->renderer2D.render(btnTriangle);
  fm.printText(Label_Cancel, 205, 407);
}
