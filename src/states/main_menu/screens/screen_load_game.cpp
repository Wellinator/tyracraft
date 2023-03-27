#include "states/main_menu/screens/screen_load_game.hpp"
#include "states/main_menu/screens/screen_main.hpp"

ScreenLoadGame::ScreenLoadGame(StateMainMenu* t_context)
    : ScreenBase(t_context) {
  t_renderer = &t_context->context->t_engine->renderer;
  init();
}

ScreenLoadGame::~ScreenLoadGame() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  textureRepo->freeBySprite(backgroundLoadGame);
  textureRepo->freeBySprite(btnTriangle);
  textureRepo->freeBySprite(btnCross);
  textureRepo->freeBySprite(btnDpadUp);
  textureRepo->freeBySprite(btnDpadDown);
  textureRepo->freeBySprite(btnSelect);
  textureRepo->freeBySprite(btnL1);
  textureRepo->freeBySprite(btnR1);

  unloadSaved();
}

void ScreenLoadGame::init() {
  loadAvailableSavesFromUSB();

  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  backgroundLoadGame.mode = Tyra::MODE_STRETCH;
  backgroundLoadGame.size.set(512, 512);
  backgroundLoadGame.position.set(0, 0);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/menu/load_game_background.png"))
      ->addLink(backgroundLoadGame.id);

  tab1.mode = Tyra::MODE_STRETCH;
  tab1.size.set(256, 256);
  tab1.position.set(128, 128);
  textureRepo->add(FileUtils::fromCwd("textures/gui/tab1.png"))
      ->addLink(tab1.id);

  toggleBtnOff.mode = Tyra::MODE_STRETCH;
  toggleBtnOff.size.set(32, 32);
  toggleBtnOff.position.set(187.0F, 165.0F);
  textureRepo->add(FileUtils::fromCwd("textures/gui/toggle_off.png"))
      ->addLink(toggleBtnOff.id);

  toggleBtnOn.mode = Tyra::MODE_STRETCH;
  toggleBtnOn.size.set(32, 32);
  toggleBtnOn.position.set(187.0F, 165.0F);
  textureRepo->add(FileUtils::fromCwd("textures/gui/toggle_on.png"))
      ->addLink(toggleBtnOn.id);

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

  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15, t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(120,
                           t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  btnDpadUp.mode = Tyra::MODE_STRETCH;
  btnDpadUp.size.set(28, 28);
  btnDpadUp.position.set(215, t_renderer->core.getSettings().getHeight() - 38);

  textureRepo->add(FileUtils::fromCwd("textures/gui/pad_up.png"))
      ->addLink(btnDpadUp.id);

  btnDpadDown.mode = Tyra::MODE_STRETCH;
  btnDpadDown.size.set(28, 28);
  btnDpadDown.position.set(310,
                           t_renderer->core.getSettings().getHeight() - 38);

  textureRepo->add(FileUtils::fromCwd("textures/gui/pad_down.png"))
      ->addLink(btnDpadDown.id);

  btnSelect.mode = Tyra::MODE_STRETCH;
  btnSelect.size.set(28, 28);
  btnSelect.position.set(400, t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_select.png"))
      ->addLink(btnSelect.id);
}

void ScreenLoadGame::update() { handleInput(); }

void ScreenLoadGame::render() {
  t_renderer->renderer2D.render(backgroundLoadGame);
  t_renderer->renderer2D.render(tab1);
  t_renderer->renderer2D.render(toggleBtnOff);
  t_renderer->renderer2D.render(btnL1);
  t_renderer->renderer2D.render(btnR1);

  renderSelectedOptions();

  // Color infoColor = Color(70, 70, 70);
  FontManager_printText(
      "Load", FontOptions(Vec2(140, 128), Color(250, 250, 250), 0.9F));
  FontManager_printText(
      "Create", FontOptions(Vec2(220, 128), Color(180, 180, 180), 0.9F));

  FontManager_printText("USB", 130.0F, 160.0F);
  FontManager_printText("MC", 210.0F, 160.0F);

  FontManager_printText(std::to_string(currentSaveIndex + 1).c_str(), 280.0F,
                        160.0F);
  FontManager_printText("/", 320.0F, 160.0F);
  FontManager_printText(std::to_string(totalOfSaves).c_str(), 335.0F, 160.0F);

  // Render saves slots
  const float iconOffsetY = 196.0F;
  const float iconHeight = 42.0F;
  if (totalOfSaves < 5) {
    for (size_t i = 0; i < totalOfSaves; i++) {
      const float iconYPosition = (i * iconHeight) + iconOffsetY - i;
      savedGamesList[i]->icon.position.set(140.0F, iconYPosition);
      t_renderer->renderer2D.render(savedGamesList[i]->icon);

      if (selectedSavedGame->id == savedGamesList[i]->id) {
        FontManager_printText(
            savedGamesList[i]->title.c_str(),
            FontOptions(Vec2(173.0F, iconYPosition), Color(255, 255, 0)));
      } else {
        FontManager_printText(savedGamesList[i]->title.c_str(), 173.0F,
                              iconYPosition);
      }
    }
  } else {
    size_t page = currentSaveIndex;
    if (currentSaveIndex + 3 == totalOfSaves) page = currentSaveIndex;

    for (size_t i = page; i < page; i++) {
      const float iconYPosition = (i * iconHeight) + iconOffsetY - i;
      savedGamesList[i]->icon.position.set(140.0F, iconYPosition);
      t_renderer->renderer2D.render(savedGamesList[i]->icon);

      if (selectedSavedGame->id == savedGamesList[i]->id) {
        FontManager_printText(
            savedGamesList[i]->title.c_str(),
            FontOptions(Vec2(173.0F, iconYPosition), Color(255, 255, 0)));
      } else {
        FontManager_printText(savedGamesList[i]->title.c_str(), 173.0F,
                              iconYPosition);
      }
    }
  }

  t_renderer->renderer2D.render(btnTriangle);
  FontManager_printText("Back", 140, 407);
  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Load", 35, 407);
  t_renderer->renderer2D.render(btnDpadUp);
  FontManager_printText("Next", 225, 407);
  t_renderer->renderer2D.render(btnDpadDown);
  FontManager_printText("Prev", 320, 407);
  t_renderer->renderer2D.render(btnSelect);
  FontManager_printText("Origin", 425, 407);
}

void ScreenLoadGame::handleInput() { handleOptionsSelection(); }

void ScreenLoadGame::handleOptionsSelection() {
  auto clickedButtons = context->context->t_engine->pad.getClicked();

  if (clickedButtons.DpadDown) {
    selectNextSave();
  } else if (clickedButtons.DpadUp) {
    selectPreviousSave();
  }

  if (clickedButtons.Select) {
    toggleSaveOrigin();
  }

  if (clickedButtons.Triangle) {
    context->playClickSound();
    backToMainMenu();
  }

  if (clickedButtons.Cross) {
    context->playClickSound();
    loadSelectedSave();
  }
}

void ScreenLoadGame::backToMainMenu() {
  context->setScreen(new ScreenMain(context));
}

void ScreenLoadGame::unloadSaved() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  for (size_t i = 0; i < savedGamesList.size(); i++) {
    textureRepo->freeBySprite(savedGamesList.at(i)->icon);
    delete savedGamesList.at(i);
  }

  selectedSavedGame = nullptr;
  currentSaveIndex = 0;
  totalOfSaves = 0;
}

void ScreenLoadGame::loadAvailableSavesFromPath(const char* fullPath) {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  std::vector<UtilDirectory> saveFilesList = Utils::listDir(fullPath);
  u8 tempId = 1;

  for (size_t i = 0; i < saveFilesList.size(); i++) {
    const UtilDirectory dir = saveFilesList.at(i);
    const std::string fileExtension =
        FileUtils::getExtensionOfFilename(dir.name);

    if (strncmp(fileExtension.c_str(), "tcw", 3) == 0) {
      TYRA_LOG("Loading save: ", dir.name.c_str());

      SaveInfoModel* model = new SaveInfoModel();

      model->id = tempId++;
      model->path = std::string(fullPath).append(dir.name);
      model->title = FileUtils::getFilenameWithoutExtension(dir.name);
      model->createdAt = dir.createdAt;

      model->icon.size.set(32, 32);
      model->icon.position.set(127, 146);
      model->icon.mode = Tyra::SpriteMode::MODE_STRETCH;

      textureRepo->add(FileUtils::fromCwd("textures/gui/save_icon.png"))
          ->addLink(model->icon.id);

      savedGamesList.push_back(model);
    }
  }

  totalOfSaves = savedGamesList.size();
  if (totalOfSaves > 0) {
    selectedSavedGame = savedGamesList.at(0);
    currentSaveIndex = 0;
  }
}

void ScreenLoadGame::loadAvailableSavesFromUSB() {
  unloadSaved();
  loadAvailableSavesFromPath(FileUtils::fromCwd("saves/").c_str());
  saveOrigin = ScreenLoadGameSaveOrigin::Mass;
}

void ScreenLoadGame::toggleSaveOrigin() {
  if (saveOrigin == ScreenLoadGameSaveOrigin::Mass) {
    loadAvailableSavesFromMemoryCard();
  } else if (saveOrigin == ScreenLoadGameSaveOrigin::MemoryCard) {
    loadAvailableSavesFromUSB();
  }
}

void ScreenLoadGame::loadAvailableSavesFromMemoryCard() {
  saveOrigin = ScreenLoadGameSaveOrigin::MemoryCard;
}

void ScreenLoadGame::renderSelectedOptions() {}

void ScreenLoadGame::selectPreviousSave() {
  u16 idx = 0;
  for (size_t i = 0; i < savedGamesList.size(); i++)
    if (savedGamesList.at(i)->id == selectedSavedGame->id) idx = i;

  if (idx == 0)
    selectedSavedGame = savedGamesList.at(savedGamesList.size() - 1);
  else
    selectedSavedGame = savedGamesList.at(idx - 1);
}

void ScreenLoadGame::selectNextSave() {
  u16 idx = 0;
  for (size_t i = 0; i < savedGamesList.size(); i++)
    if (savedGamesList.at(i)->id == selectedSavedGame->id) idx = i;

  if (idx == savedGamesList.size() - 1)
    selectedSavedGame = savedGamesList.at(0);
  else
    selectedSavedGame = savedGamesList.at(idx + 1);
}

void ScreenLoadGame::loadSelectedSave() {
  const NewGameOptions model = NewGameOptions();
  // TODO: Read savefile and load world
  context->loadSavedGame(model);
}
