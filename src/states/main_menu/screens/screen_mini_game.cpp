#include "states/main_menu/screens/screen_mini_game.hpp"
#include "states/main_menu/screens/screen_main.hpp"
#include "managers/save_manager.hpp"

ScreenMiniGame::ScreenMiniGame(StateMainMenu* t_context)
    : ScreenBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->init();
}

ScreenMiniGame::~ScreenMiniGame() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  textureRepo->free(slotTexture->id);
  textureRepo->free(slotActiveTexture->id);
  textureRepo->free(btnTriangleTexture->id);

  textureRepo->freeBySprite(backgroundNewGame);
  textureRepo->freeBySprite(tab3);
  textureRepo->freeBySprite(slotTextureActive);
  textureRepo->freeBySprite(btnCross);
  textureRepo->freeBySprite(btnSquare);
  textureRepo->freeBySprite(btnStart);
  textureRepo->freeBySprite(btnCircle);
  textureRepo->freeBySprite(btnL1);
  textureRepo->freeBySprite(btnR1);
  textureRepo->freeBySprite(btnDpadLeft);
  textureRepo->freeBySprite(btnDpadRight);
  textureRepo->freeBySprite(dialogWindow);
  textureRepo->freeBySprite(overlay);

  for (size_t i = 0; i < miniGames.size(); i++) {
    textureRepo->freeBySprite(miniGames[i]->icon);
    delete miniGames[i];
  }

  unloadSaved();
}

void ScreenMiniGame::update(const float& deltaTime) {
  // fpsCounter++;
  this->handleInput();
  // fpsCounter %= 50;
}

void ScreenMiniGame::render() {
  this->t_renderer->renderer2D.render(backgroundNewGame);
  this->t_renderer->renderer2D.render(tab3);
  this->t_renderer->renderer2D.render(btnL1);
  this->t_renderer->renderer2D.render(btnR1);

  this->renderSelectedOptions();

  FontManager_printText(
      Label_Load, FontOptions(Vec2(140, 128), Color(180, 180, 180), 0.9F));
  FontManager_printText(
      Label_Create, FontOptions(Vec2(220, 128), Color(250, 250, 250), 0.9F));
  FontManager_printText(
      Label_Mini, FontOptions(Vec2(310, 128), Color(250, 250, 250), 0.9F));

  Color infoColor = Color(70, 70, 70);
  FontManager_printText(Label_MiniGame,
                        FontOptions(Vec2(185, 157), infoColor, 0.8F));

  if (selectedMiniGame) {
    this->t_renderer->renderer2D.render(selectedMiniGame->icon);
    FontManager_printText(selectedMiniGame->title.c_str(),
                          FontOptions(Vec2(185.0f, 185.0f), infoColor, 0.8F));
  }

  FontOptions options;
  options.position = Vec2(242, 220);
  options.alignment = TextAlignment::Center;
  FontManager_printText(Label_NewGame, options);

  options.position = Vec2(242, 258);
  options.alignment = TextAlignment::Center;
  FontManager_printText(Label_Continue, options);

  options.alignment = TextAlignment::Center;
  options.position.set(248, 295);
  FontManager_printText(Label_ResetProgress, options);

  // switch (this->model.type) {
  //   case WorldType::WORLD_TYPE_ORIGINAL:
  //     worldTypeOptions.position.set(248, 295);
  //     FontManager_printText(Label_WorldTypeOriginal, worldTypeOptions);
  //     break;
  //   case WorldType::WORLD_TYPE_FLAT:
  //     worldTypeOptions.position.set(248, 295);
  //     FontManager_printText(Label_WorldTypeFlat, worldTypeOptions);
  //     break;
  //   case WorldType::WORLD_TYPE_ISLAND:
  //     worldTypeOptions.position.set(248, 295);
  //     FontManager_printText(Label_WorldTypeIsland, worldTypeOptions);
  //     break;
  //   case WorldType::WORLD_TYPE_WOODS:
  //     worldTypeOptions.position.set(248, 295);
  //     FontManager_printText(Label_WorldTypeWoods, worldTypeOptions);
  //     break;
  //   case WorldType::WORLD_TYPE_FLOATING:
  //     worldTypeOptions.position.set(248, 295);
  //     FontManager_printText(Label_WorldTypeFloating, worldTypeOptions);
  //     break;
  //   case WorldType::WORLD_MINI_GAME_MAZECRAFT:
  //     worldTypeOptions.position.set(248, 295);
  //     FontManager_printText(Label_WorldTypeMazecraft, worldTypeOptions);
  //     break;

  //   default:
  //     break;
  // }

  if (activeOption == ScreenMiniGameOptions::MiniGameSelection) {
    this->t_renderer->renderer2D.render(btnDpadLeft);
    this->t_renderer->renderer2D.render(btnDpadRight);
    this->t_renderer->renderer2D.render(btnTriangleTexturePack);

    FontManager_printText(Label_Prev, 35, 407);
    FontManager_printText(Label_Next, 160, 407);
    FontManager_printText(Label_Back, 280, 407);
  } else {
    this->t_renderer->renderer2D.render(btnTriangle);
    this->t_renderer->renderer2D.render(btnCross);

    FontManager_printText(Label_Select, 35, 407);
    FontManager_printText(Label_Back, 160, 407);
  }

  if (displayPreviousSavePresent) {
    renderPreviousSavePresentDialog();
  }

  if (displayProgressReseted) {
    renderProgressResetedDialog();
  }
}

void ScreenMiniGame::init() {
  getAvailableMiniGames();

  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  slotTexture = textureRepo->add(FileUtils::fromCwd("textures/gui/slot.png"));
  slotActiveTexture =
      textureRepo->add(FileUtils::fromCwd("textures/gui/slot_active.png"));

  backgroundNewGame.mode = Tyra::MODE_STRETCH;
  backgroundNewGame.size.set(512, 512);
  backgroundNewGame.position.set(0, 0);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/menu/load_game_background.png"))
      ->addLink(backgroundNewGame.id);

  tab3.mode = Tyra::MODE_STRETCH;
  tab3.size.set(256, 256);
  tab3.position.set(128, 128);
  textureRepo->add(FileUtils::fromCwd("textures/gui/tab3.png"))
      ->addLink(tab3.id);

  btnL1.mode = Tyra::MODE_STRETCH;
  btnL1.size.set(32, 32);
  btnL1.position.set(100.0F, 132.0F);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_L1.png"))
      ->addLink(btnL1.id);

  btnR1.mode = Tyra::MODE_STRETCH;
  btnR1.size.set(32, 32);
  btnR1.position.set(390.0F, 132.0F);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_R1.png"))
      ->addLink(btnR1.id);

  slotNewGame.mode = Tyra::MODE_STRETCH;
  slotNewGame.size.set(slotWidth, slotHeight);
  slotNewGame.position.set(133, 218);
  slotTexture->addLink(slotNewGame.id);

  slotNewGameActive.mode = Tyra::MODE_STRETCH;
  slotNewGameActive.size.set(slotWidth, slotHeight);
  slotNewGameActive.position.set(133, 218);
  slotActiveTexture->addLink(slotNewGameActive.id);

  slotLoadGame.mode = Tyra::MODE_STRETCH;
  slotLoadGame.size.set(slotWidth, slotHeight);
  slotLoadGame.position.set(133, 255);
  slotTexture->addLink(slotLoadGame.id);

  slotLoadGameActive.mode = Tyra::MODE_STRETCH;
  slotLoadGameActive.size.set(slotWidth, slotHeight);
  slotLoadGameActive.position.set(133, 255);
  slotActiveTexture->addLink(slotLoadGameActive.id);

  slotResetProgress.mode = Tyra::MODE_STRETCH;
  slotResetProgress.size.set(slotWidth, slotHeight);
  slotResetProgress.position.set(133, 292);
  slotTexture->addLink(slotResetProgress.id);

  slotResetProgressActive.mode = Tyra::MODE_STRETCH;
  slotResetProgressActive.size.set(slotWidth, slotHeight);
  slotResetProgressActive.position.set(133, 292);
  slotActiveTexture->addLink(slotResetProgressActive.id);

  slotTextureActive.mode = Tyra::MODE_STRETCH;
  slotTextureActive.size.set(52, 52);
  slotTextureActive.position.set(133, 156);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/slot_texture_pack_active.png"))
      ->addLink(slotTextureActive.id);

  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnStart.mode = Tyra::MODE_STRETCH;
  btnStart.size.set(25, 25);
  btnStart.position.set(380,
                        this->t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_start.png"))
      ->addLink(btnStart.id);

  btnSquare.mode = Tyra::MODE_STRETCH;
  btnSquare.size.set(25, 25);
  btnSquare.position.set(260,
                         this->t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_square.png"))
      ->addLink(btnSquare.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(
      140, this->t_renderer->core.getSettings().getHeight() - 40);

  btnTriangleTexturePack.mode = Tyra::MODE_STRETCH;
  btnTriangleTexturePack.size.set(25, 25);
  btnTriangleTexturePack.position.set(
      260, this->t_renderer->core.getSettings().getHeight() - 40);

  btnTriangleTexture =
      textureRepo->add(FileUtils::fromCwd("textures/gui/btn_triangle.png"));
  btnTriangleTexture->addLink(btnTriangle.id);
  btnTriangleTexture->addLink(btnTriangleTexturePack.id);

  btnCircle.mode = Tyra::MODE_STRETCH;
  btnCircle.size.set(25, 25);
  btnCircle.position.set(260,
                         this->t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_circle.png"))
      ->addLink(btnCircle.id);

  btnDpadLeft.mode = Tyra::MODE_STRETCH;
  btnDpadLeft.size.set(28, 28);
  btnDpadLeft.position.set(
      15, this->t_renderer->core.getSettings().getHeight() - 33);

  textureRepo->add(FileUtils::fromCwd("textures/gui/pad_left.png"))
      ->addLink(btnDpadLeft.id);

  btnDpadRight.mode = Tyra::MODE_STRETCH;
  btnDpadRight.size.set(28, 28);
  btnDpadRight.position.set(
      133, this->t_renderer->core.getSettings().getHeight() - 33);

  textureRepo->add(FileUtils::fromCwd("textures/gui/pad_right.png"))
      ->addLink(btnDpadRight.id);

  dialogWindow.mode = Tyra::MODE_STRETCH;
  dialogWindow.size.set(256, 256);
  dialogWindow.position.set(128, 120);
  textureRepo->add(FileUtils::fromCwd("textures/gui/window.png"))
      ->addLink(dialogWindow.id);

  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(512, 448);
  overlay.position.set(0, 0);
  textureRepo->add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);
}

void ScreenMiniGame::handleInput() { handleOptionsSelection(); }

void ScreenMiniGame::handleOptionsSelection() {
  auto clickedButtons = this->context->context->t_engine->pad.getClicked();

  if (displayPreviousSavePresent) {
    if (clickedButtons.Cross) {
      this->context->playClickSound();
      displayPreviousSavePresent = false;
      activeOption = ScreenMiniGameOptions::NewGame;
    }
    return;
  } else if (displayProgressReseted) {
    if (clickedButtons.Cross) {
      this->context->playClickSound();
      displayProgressReseted = false;
      activeOption = ScreenMiniGameOptions::NewGame;
    }
    return;
  }

  if (clickedButtons.L1) {
    context->setScreen(new ScreenNewGame(context));
  } else if (clickedButtons.R1) {
    context->setScreen(new ScreenLoadGame(context));
  }

  if (clickedButtons.DpadDown) {
    int nextOption = (int)this->activeOption + 1;
    if (nextOption > (u8)ScreenMiniGameOptions::ResetProgress)
      this->activeOption = ScreenMiniGameOptions::MiniGameSelection;
    else
      this->activeOption = static_cast<ScreenMiniGameOptions>(nextOption);
  } else if (clickedButtons.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = ScreenMiniGameOptions::ResetProgress;
    else
      this->activeOption = static_cast<ScreenMiniGameOptions>(nextOption);
  }

  if (activeOption == ScreenMiniGameOptions::MiniGameSelection) {
    if (clickedButtons.DpadLeft)
      selectPreviousMiniGame();
    else if (clickedButtons.DpadRight)
      selectNextMiniGame();
  }

  if (clickedButtons.Triangle) {
    this->context->playClickSound();
    this->backToMainMenu();
  } else if (clickedButtons.Cross) {
    this->context->playClickSound();
    this->selectedOption = this->activeOption;
    this->updateModel();

    if (this->selectedOption == ScreenMiniGameOptions::NewGame) {
      TYRA_LOG("NewGame");
      if (isThereMiniGameSavedData()) {
        displayPreviousSavePresent = true;
      } else {
        return createNewWorld();
      }
    } else if (this->selectedOption == ScreenMiniGameOptions::LoadGame) {
      if (isThereMiniGameSavedData()) {
        TYRA_LOG("LoadGame");
        loadAvailableSaveFromPath();
        if (savedGame)
          return context->loadSavedMiniGame(GameMode::Maze, savedGame->path);
      }

      TYRA_LOG("Display message: NO SAVED GAME DATA");
    }

    else if (this->selectedOption == ScreenMiniGameOptions::ResetProgress) {
      TYRA_LOG("Deleting progress...");
      deleteProgress();
    }
  }
}

void ScreenMiniGame::updateModel() {
  if (this->activeOption == ScreenMiniGameOptions::MiniGameSelection) {
    if (selectedMiniGame != nullptr &&
        selectedMiniGame->id == (u8)MiniGames::MazeCraft) {
      this->model.type = WorldType::WORLD_MINI_GAME_MAZECRAFT;
    }
  }
}

void ScreenMiniGame::backToMainMenu() {
  this->context->setScreen(new ScreenMain(this->context));
}

void ScreenMiniGame::unloadSaved() { delete savedGame; }

void ScreenMiniGame::loadAvailableSaveFromPath() {
  std::string tempSaveFileName = FileUtils::fromCwd(
      "saves/" + inputWorldName + "." + MINIGAME_FILE_EXTENSION);

  TYRA_LOG("Loading saved mini game: ", inputWorldName);

  savedGame = new SaveInfoModel();

  savedGame->id = 1;
  savedGame->name = inputWorldName;
  savedGame->path = tempSaveFileName;

  SaveManager::SetSaveInfo(savedGame->path.c_str(), savedGame);

  TYRA_LOG("Version: ", savedGame->version);
  TYRA_LOG("name: ", savedGame->name.c_str());

  // Make sure it'll only load from version 1 above
  savedGame->valid = savedGame->version > 0;

  return;
}

bool ScreenMiniGame::isThereMiniGameSavedData() {
  std::string tempSaveFileName = FileUtils::fromCwd(
      "saves/" + inputWorldName + "." + MINIGAME_FILE_EXTENSION);

  return SaveManager::CheckIfSaveExist(tempSaveFileName.c_str());
}

void ScreenMiniGame::createNewWorld() {
  model.name = inputWorldName;
  model.seed = 1;
  model.texturePack = "default";

  switch (model.type) {
    case WorldType::WORLD_MINI_GAME_MAZECRAFT:
      model.gameMode = GameMode::Maze;
      break;

    default:
      return TYRA_ERROR("Not valid Mini Game");
  }
  context->createMiniGame(model);
}

void ScreenMiniGame::deleteProgress() {
  std::string tempSaveFileName = FileUtils::fromCwd(
      "saves/" + inputWorldName + "." + MINIGAME_FILE_EXTENSION);

  SaveManager::DeleteSave(tempSaveFileName.c_str());
  displayProgressReseted = !isThereMiniGameSavedData();
}

void ScreenMiniGame::renderSelectedOptions() {
  t_renderer->renderer2D.render(slotNewGame);
  t_renderer->renderer2D.render(slotLoadGame);
  t_renderer->renderer2D.render(slotResetProgress);

  if (activeOption == ScreenMiniGameOptions::MiniGameSelection)
    t_renderer->renderer2D.render(slotTextureActive);
  else if (activeOption == ScreenMiniGameOptions::NewGame)
    t_renderer->renderer2D.render(slotNewGameActive);
  else if (activeOption == ScreenMiniGameOptions::LoadGame)
    t_renderer->renderer2D.render(slotLoadGameActive);
  else if (activeOption == ScreenMiniGameOptions::ResetProgress)
    t_renderer->renderer2D.render(slotResetProgressActive);
}

void ScreenMiniGame::getAvailableMiniGames() {
  loadMazeCraftMiniGameInfo();
  if (!selectedMiniGame && miniGames.size()) {
    selectedMiniGame = miniGames[0];

    if (selectedMiniGame->id == (u8)MiniGames::MazeCraft) {
      model.gameMode = GameMode::Maze;
      model.type = WorldType::WORLD_MINI_GAME_MAZECRAFT;
    }
  }
}

void ScreenMiniGame::loadMazeCraftMiniGameInfo() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  MiniGameInfoModel* model = new MiniGameInfoModel();

  model->id = (u8)MiniGames::MazeCraft;
  model->title = std::string("MazeCraft");
  model->description =
      std::string("Get the right path and find the jack o'lantern.");

  model->icon.size.set(48, 48);
  model->icon.position.set(135, 158);
  model->icon.mode = Tyra::SpriteMode::MODE_STRETCH;

  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/mini_game/mazecraft_icon.png"))
      ->addLink(model->icon.id);

  miniGames.push_back(model);
}

void ScreenMiniGame::selectPreviousMiniGame() {
  u16 idx = 0;
  for (size_t i = 0; i < miniGames.size(); i++)
    if (miniGames[i]->id == selectedMiniGame->id) idx = i;

  if (idx == 0)
    selectedMiniGame = miniGames[miniGames.size() - 1];
  else
    selectedMiniGame = miniGames[idx - 1];
}

void ScreenMiniGame::selectNextMiniGame() {
  u16 idx = 0;
  for (size_t i = 0; i < miniGames.size(); i++)
    if (miniGames[i]->id == selectedMiniGame->id) idx = i;

  if (idx == miniGames.size() - 1)
    selectedMiniGame = miniGames[0];
  else
    selectedMiniGame = miniGames[idx + 1];
}

void ScreenMiniGame::renderPreviousSavePresentDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  FontManager_printText(Label_Ops, titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 180);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  FontManager_printText(Label_PreviousSavePresentErrorPart1, dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText(Label_PreviousSavePresentErrorPart2, dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText(Label_PreviousSavePresentErrorPart3, dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(Label_Confirm, 40, 407);
}

void ScreenMiniGame::renderProgressResetedDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  FontManager_printText(Label_Success, titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 180);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  FontManager_printText(Label_PreviousSaveDeletedSuccessPart1, dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText(Label_PreviousSaveDeletedSuccessPart2, dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(Label_Confirm, 40, 407);
}
