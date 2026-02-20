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

  // Backgrund
  background.mode = Tyra::MODE_STRETCH;
  background.size.set(512, 512);
  background.position.set(0, 0);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/menu/load_game_background.png"))
      ->addLink(background.id);

  // OverlayL
  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(halfWidth * 2, halfHeight * 2);
  overlay.position.set(0, 0);

  textureRepo->add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);

  // Load slots: row0=DrawDistance(text only), row1=FpsMode, row2=SaveGame, row3=Quit
  raw_slot[0].mode = Tyra::MODE_STRETCH;
  raw_slot[0].size.set(SLOT_WIDTH, 35);
  raw_slot[0].position.set(halfWidth - SLOT_WIDTH / 2, 200);
  raw_slot[1].mode = Tyra::MODE_STRETCH;
  raw_slot[1].size.set(SLOT_WIDTH, 35);
  raw_slot[1].position.set(halfWidth - SLOT_WIDTH / 2, 240);
  raw_slot[2].mode = Tyra::MODE_STRETCH;
  raw_slot[2].size.set(SLOT_WIDTH, 35);
  raw_slot[2].position.set(halfWidth - SLOT_WIDTH / 2, 280);

  this->textureRawSlot =
      textureRepo->add(FileUtils::fromCwd("textures/gui/slot.png"));

  this->textureRawSlot->addLink(raw_slot[0].id);
  this->textureRawSlot->addLink(raw_slot[1].id);
  this->textureRawSlot->addLink(raw_slot[2].id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 200);
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
}

void StateGameMenu::update(const float& deltaTime) {
  this->handleInput(deltaTime);
  this->hightLightActiveOption();
  this->navigate();
}

void StateGameMenu::render() {
  const float halfWidth = this->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = this->t_renderer->core.getSettings().getHeight() / 2;
  FontManager& fm = FontManager::getInstanceRef();

  t_renderer->renderer2D.render(background);

  FontOptions drawDistanceLabel;
  drawDistanceLabel.position.set(248, 155);
  drawDistanceLabel.alignment = TextAlignment::Center;

  if (activeOption == GameMenuOptions::DrawDistance)
    drawDistanceLabel.color.set(128, 128, 0);
  fm.printText(Label_DrawDistance + ": " + getDrawDistanceModeLabel(),
               drawDistanceLabel);

  // FPS Mode row
  t_renderer->renderer2D.render(raw_slot[0]);
  FontOptions fpsModeLabel;
  fpsModeLabel.position.set(248, 200 + 3);
  fpsModeLabel.alignment = TextAlignment::Center;
  if (activeOption == GameMenuOptions::FpsModeOption)
    fpsModeLabel.color.set(128, 128, 0);
  fm.printText(Label_FpsMode + ": " + getFpsModeLabel(), fpsModeLabel);

  t_renderer->renderer2D.render(raw_slot[1]);
  t_renderer->renderer2D.render(raw_slot[2]);
  if (activeOption != GameMenuOptions::DrawDistance &&
      activeOption != GameMenuOptions::FpsModeOption)
    t_renderer->renderer2D.render(active_slot);

  fm.printText(Label_GameMenu, halfWidth - 64, halfHeight - 200);

  FontOptions saveGameLabel;
  saveGameLabel.position.set(246, 240 + 3);
  saveGameLabel.alignment = TextAlignment::Center;
  if (activeOption == GameMenuOptions::SaveGame)
    saveGameLabel.color.set(128, 128, 0);
  fm.printText(Label_Save, saveGameLabel);

  FontOptions quitToTitleLabel;
  quitToTitleLabel.position.set(246, 280 + 3);
  quitToTitleLabel.alignment = TextAlignment::Center;
  if (activeOption == GameMenuOptions::Quit)
    quitToTitleLabel.color.set(128, 128, 0);
  fm.printText(Label_Quit, quitToTitleLabel);

  if (needSaveOverwriteConfirmation) {
    renderSaveOverwritingDialog();
  } else if (needQuitConfirmation) {
    renderQuitWithoutSavingDialog();
  } else {
    t_renderer->renderer2D.render(btnCross);
    fm.printText(Label_Select, 40, 407);
    t_renderer->renderer2D.render(btnStart);
    fm.printText(Label_BackToGame, 205, 407);
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

  if (clicked.DpadDown) {
    int nextOption = (int)this->activeOption + 1;
    if (nextOption > (int)GameMenuOptions::Quit)
      this->activeOption = GameMenuOptions::DrawDistance;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);

  } else if (clicked.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = GameMenuOptions::Quit;
    else
      this->activeOption = static_cast<GameMenuOptions>(nextOption);
  }

  if (activeOption == GameMenuOptions::DrawDistance) {
    if (clicked.DpadLeft)
      cycleDrawDistanceMode(-1);
    else if (clicked.DpadRight)
      cycleDrawDistanceMode(1);
  } else if (activeOption == GameMenuOptions::FpsModeOption) {
    if (clicked.DpadLeft)
      cycleFpsMode(-1);
    else if (clicked.DpadRight)
      cycleFpsMode(1);
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
    } else if (activeOption == GameMenuOptions::Quit) {
      this->playClickSound();
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
  textureRepository->freeBySprite(background);
  textureRepository->freeBySprite(overlay);
  textureRepository->freeBySprite(active_slot);
  textureRepository->freeBySprite(btnCross);
  textureRepository->freeBySprite(btnTriangle);
  textureRepository->freeBySprite(dialogWindow);
  textureRepository->freeBySprite(btnStart);
}

void StateGameMenu::playClickSound() {
  SoundManager* pSoundManager = SoundManager::getInstance();

  const s8 ch = pSoundManager->getAvailableChannel();
  this->stateGamePlay->context->t_engine->audio.adpcm.setVolume(60, ch);
  pSoundManager->playSfx(SoundFxCategory::Random, SoundFX::WoodClick, ch);
}

void StateGameMenu::hightLightActiveOption() {
  if (this->activeOption == GameMenuOptions::SaveGame) {
    this->active_slot.position.y = (1 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
  } else if (this->activeOption == GameMenuOptions::Quit) {
    this->active_slot.position.y = (2 * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
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
