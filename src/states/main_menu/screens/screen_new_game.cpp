#include "states/main_menu/screens/screen_new_game.hpp"
#include "states/main_menu/screens/screen_main.hpp"
#include "managers/save_manager.hpp"

ScreenNewGame::ScreenNewGame(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->inputSeed = getSeed();
  this->init();
}

ScreenNewGame::~ScreenNewGame() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  textureRepo->free(slotTexture->id);
  textureRepo->free(slotActiveTexture->id);
  textureRepo->free(btnTriangleTexture->id);

  textureRepo->freeBySprite(backgroundNewGame);
  textureRepo->freeBySprite(tab2);
  textureRepo->freeBySprite(slotSeedInput);
  textureRepo->freeBySprite(slotWorldNameInput);
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

  for (size_t i = 0; i < texturePacks.size(); i++) {
    textureRepo->freeBySprite(texturePacks.at(i)->icon);
    delete texturePacks.at(i);
  }
}

void ScreenNewGame::update() {
  fpsCounter++;
  this->handleInput();
  fpsCounter %= 50;
}

void ScreenNewGame::render() {
  this->t_renderer->renderer2D.render(&backgroundNewGame);
  this->t_renderer->renderer2D.render(tab2);
  this->t_renderer->renderer2D.render(btnL1);
  this->t_renderer->renderer2D.render(btnR1);

  this->renderSelectedOptions();

  FontManager_printText(
      "Load", FontOptions(Vec2(140, 128), Color(180, 180, 180), 0.9F));
  FontManager_printText(
      "Create", FontOptions(Vec2(220, 128), Color(250, 250, 250), 0.9F));

  Color infoColor = Color(70, 70, 70);
  FontManager_printText("Texture Packs: ",
                        FontOptions(Vec2(190, 157), infoColor, 0.8F));
  FontManager_printText("By: ", FontOptions(Vec2(190, 187), infoColor, 0.8F));

  if (selectedTexturePack) {
    this->t_renderer->renderer2D.render(selectedTexturePack->icon);
    FontManager_printText(selectedTexturePack->title.c_str(),
                          FontOptions(Vec2(190, 172), infoColor, 0.8F));
    FontManager_printText(selectedTexturePack->author.c_str(),
                          FontOptions(Vec2(230, 187), infoColor, 0.8F));
  }

  if (isEditingWorldName) {
    FontOptions options;
    options.position = Vec2(130, 220);
    options.color = Color(168, 160, 50);

    if (fpsCounter < 25) {
      FontManager_printText(this->tempWorldName.c_str(), options);
    } else {
      FontManager_printText(this->tempWorldNameMask.c_str(), options);
    }

  } else {
    FontOptions options;
    options.position = Vec2(242, 220);
    options.alignment = TextAlignment::Center;
    FontManager_printText(inputWorldName, options);
  }

  if (isEditingSeed) {
    FontOptions options;
    options.position = Vec2(175, 258);
    options.color = Color(168, 160, 50);

    if (fpsCounter < 25) {
      FontManager_printText(this->tempSeed.c_str(), options);
    } else {
      FontManager_printText(this->tempSeedMask.c_str(), options);
    }

  } else {
    FontOptions options;
    options.position = Vec2(242, 258);
    options.alignment = TextAlignment::Center;
    FontManager_printText("Seed: " + inputSeed, options);
  }

  switch (this->model.type) {
    case WorldType::WORLD_TYPE_ORIGINAL:
      FontManager_printText("World Type: Original", 130, 295);
      break;
    case WorldType::WORLD_TYPE_FLAT:
      FontManager_printText("World Type: Flat", 150, 295);
      break;
    case WorldType::WORLD_TYPE_ISLAND:
      FontManager_printText("World Type: Island", 135, 295);
      break;
    case WorldType::WORLD_TYPE_WOODS:
      FontManager_printText("World Type: Woods", 140, 295);
      break;
    case WorldType::WORLD_TYPE_FLOATING:
      FontManager_printText("World Type: Floating", 130, 295);
      break;

    default:
      break;
  }

  FontManager_printText("Create New World", 145, 332);

  if (isEditingSeed) {
    this->t_renderer->renderer2D.render(&btnTriangle);
    this->t_renderer->renderer2D.render(&btnCross);

    FontManager_printText("Confirm", 35, 407);
    FontManager_printText("Cancel", 160, 407);
  } else if (isEditingWorldName) {
    this->t_renderer->renderer2D.render(&btnCross);
    this->t_renderer->renderer2D.render(&btnTriangle);
    this->t_renderer->renderer2D.render(&btnSquare);
    this->t_renderer->renderer2D.render(&btnStart);

    FontManager_printText("Select", 35, 407);
    FontManager_printText("Cancel", 160, 407);
    FontManager_printText("Bksp", 285, 407);
    FontManager_printText("Confirm", 405, 407);
  } else {
    if (activeOption == ScreenNewGameOptions::TexturePack) {
      this->t_renderer->renderer2D.render(&btnDpadLeft);
      this->t_renderer->renderer2D.render(&btnDpadRight);
      this->t_renderer->renderer2D.render(&btnTriangleTexturePack);

      FontManager_printText("Prev", 35, 407);
      FontManager_printText("Next", 160, 407);
      FontManager_printText("Back", 280, 407);
    } else {
      this->t_renderer->renderer2D.render(&btnTriangle);
      this->t_renderer->renderer2D.render(&btnCross);

      if (activeOption == ScreenNewGameOptions::Seed) {
        FontManager_printText("Edit", 35, 407);
        FontManager_printText("Random", 280, 407);
        this->t_renderer->renderer2D.render(&btnCircle);
      } else if (activeOption == ScreenNewGameOptions::WorldName) {
        FontManager_printText("Edit", 35, 407);
      } else {
        FontManager_printText("Select", 35, 407);
      }
      FontManager_printText("Back", 160, 407);
    }
  }

  if (needToChangeWorldName) {
    renderWorldNameDialog();
  }
}

void ScreenNewGame::init() {
  getAvailableTexturePacks();

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

  tab2.mode = Tyra::MODE_STRETCH;
  tab2.size.set(256, 256);
  tab2.position.set(128, 128);
  textureRepo->add(FileUtils::fromCwd("textures/gui/tab2.png"))
      ->addLink(tab2.id);

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

  slotWorldName.mode = Tyra::MODE_STRETCH;
  slotWorldName.size.set(slotWidth, slotHeight);
  slotWorldName.position.set(133, 218);
  slotTexture->addLink(slotWorldName.id);

  slotWorldNameActive.mode = Tyra::MODE_STRETCH;
  slotWorldNameActive.size.set(slotWidth, slotHeight);
  slotWorldNameActive.position.set(133, 218);
  slotActiveTexture->addLink(slotWorldNameActive.id);

  slotWorldNameInput.mode = Tyra::MODE_STRETCH;
  slotWorldNameInput.size.set(slotWidth, slotHeight);
  slotWorldNameInput.position.set(133, 218);
  textureRepo->add(FileUtils::fromCwd("textures/gui/slot_input.png"))
      ->addLink(slotWorldNameInput.id);

  slotSeed.mode = Tyra::MODE_STRETCH;
  slotSeed.size.set(slotWidth, slotHeight);
  slotSeed.position.set(133, 255);
  slotTexture->addLink(slotSeed.id);

  slotSeedActive.mode = Tyra::MODE_STRETCH;
  slotSeedActive.size.set(slotWidth, slotHeight);
  slotSeedActive.position.set(133, 255);
  slotActiveTexture->addLink(slotSeedActive.id);

  slotSeedInput.mode = Tyra::MODE_STRETCH;
  slotSeedInput.size.set(slotWidth, slotHeight);
  slotSeedInput.position.set(133, 255);
  textureRepo->add(FileUtils::fromCwd("textures/gui/slot_input.png"))
      ->addLink(slotSeedInput.id);

  slotWorldType.mode = Tyra::MODE_STRETCH;
  slotWorldType.size.set(slotWidth, slotHeight);
  slotWorldType.position.set(133, 292);
  slotTexture->addLink(slotWorldType.id);

  slotWorldTypeActive.mode = Tyra::MODE_STRETCH;
  slotWorldTypeActive.size.set(slotWidth, slotHeight);
  slotWorldTypeActive.position.set(133, 292);
  slotActiveTexture->addLink(slotWorldTypeActive.id);

  slotCreateNewWorld.mode = Tyra::MODE_STRETCH;
  slotCreateNewWorld.size.set(slotWidth, slotHeight);
  slotCreateNewWorld.position.set(133, 329);
  slotTexture->addLink(slotCreateNewWorld.id);

  slotCreateNewWorldActive.mode = Tyra::MODE_STRETCH;
  slotCreateNewWorldActive.size.set(slotWidth, slotHeight);
  slotCreateNewWorldActive.position.set(133, 329);
  slotActiveTexture->addLink(slotCreateNewWorldActive.id);

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

void ScreenNewGame::handleInput() {
  if (isEditingSeed) {
    handleSeedInput();
  } else if (isEditingWorldName) {
    handleWorldNameInput();
  } else {
    handleOptionsSelection();
  }
}

void ScreenNewGame::handleOptionsSelection() {
  auto clickedButtons = this->context->context->t_engine->pad.getClicked();

  if (needToChangeWorldName) {
    if (clickedButtons.Cross) {
      this->context->playClickSound();
      needToChangeWorldName = false;
      activeOption = ScreenNewGameOptions::WorldName;
    }
    return;
  }

  if (clickedButtons.L1 || clickedButtons.R1) {
    context->setScreen(new ScreenLoadGame(context));
  }

  if (clickedButtons.DpadDown) {
    int nextOption = (int)this->activeOption + 1;
    if (nextOption > (u8)ScreenNewGameOptions::CreateNewWorld)
      this->activeOption = ScreenNewGameOptions::TexturePack;
    else
      this->activeOption = static_cast<ScreenNewGameOptions>(nextOption);
  } else if (clickedButtons.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = ScreenNewGameOptions::CreateNewWorld;
    else
      this->activeOption = static_cast<ScreenNewGameOptions>(nextOption);
  }

  if (activeOption == ScreenNewGameOptions::TexturePack) {
    if (clickedButtons.DpadLeft)
      selectPreviousTexturePack();
    else if (clickedButtons.DpadRight)
      selectNextTexturePack();
  }

  if (clickedButtons.Triangle) {
    this->context->playClickSound();
    this->backToMainMenu();
  } else if (clickedButtons.Cross) {
    this->context->playClickSound();
    this->selectedOption = this->activeOption;
    this->updateModel();
    if (this->selectedOption == ScreenNewGameOptions::CreateNewWorld) {
      if (canCreateANewWorldWithCurrentName()) {
        createNewWorld();
      } else {
        needToChangeWorldName = true;
      }
      return;
    } else if (this->selectedOption == ScreenNewGameOptions::WorldName)
      startEditingWorldName();
    else if (this->selectedOption == ScreenNewGameOptions::Seed)
      startEditingSeed();
  } else if (clickedButtons.Circle &&
             activeOption == ScreenNewGameOptions::Seed) {
    inputSeed = getSeed();
  }
}

void ScreenNewGame::handleSeedInput() {
  auto clickedButtons = this->context->context->t_engine->pad.getClicked();
  auto validChars = NumericValidChars;
  std::size_t found = validChars.find(tempSeed[editingIndex]);
  u8 currentCharIndex = found != std::string::npos ? found : 0;

  if (clickedButtons.DpadDown) {
    u16 newCharIndex = currentCharIndex - 1 < 0 ? validChars.length() - 1
                                                : currentCharIndex - 1;

    tempSeed[editingIndex] = validChars[newCharIndex];
  } else if (clickedButtons.DpadUp) {
    u16 newCharIndex = currentCharIndex + 1 >= (u16)validChars.length()
                           ? 0
                           : currentCharIndex + 1;

    tempSeed[editingIndex] = validChars[newCharIndex];
  } else if (clickedButtons.DpadLeft) {
    s8 prev = editingIndex - 1;
    editingIndex = prev < 0 ? (u8)(tempSeed.length() - 1) : (u8)prev;
  } else if (clickedButtons.DpadRight) {
    u8 next = editingIndex + 1;
    editingIndex = next > tempSeed.length() - 1 ? 0 : (u8)next;
  }

  updateTempSeedMask();

  if (clickedButtons.Triangle) {
    cancelEditingSeed();
  } else if (clickedButtons.Cross) {
    saveSeed();
  }
}

void ScreenNewGame::handleWorldNameInput() {
  auto clickedButtons = this->context->context->t_engine->pad.getClicked();
  auto validChars = SpecialValidChars + AlphanumericValidChars;
  std::size_t found = validChars.find(tempWorldName[editingIndexWorldName]);
  u8 currentCharIndex = found != std::string::npos ? found : 0;

  if (clickedButtons.DpadDown) {
    u16 newCharIndex = currentCharIndex - 1 < 0 ? validChars.length() - 1
                                                : currentCharIndex - 1;

    tempWorldName[editingIndexWorldName] = validChars[newCharIndex];
  } else if (clickedButtons.DpadUp) {
    u16 newCharIndex = currentCharIndex + 1 >= (u16)validChars.length()
                           ? 0
                           : currentCharIndex + 1;

    tempWorldName[editingIndexWorldName] = validChars[newCharIndex];
  }

  updateTempWorldNameMask();

  if (clickedButtons.Triangle) {
    cancelEditingWorldName();
  } else if (clickedButtons.Cross) {
    addWorldNameLastChar();
  } else if (clickedButtons.Square) {
    removeWorldNameLastChar();
  } else if (clickedButtons.Start) {
    saveWorldName();
  }
}

void ScreenNewGame::updateModel() {
  if (this->activeOption == ScreenNewGameOptions::WorldType) {
    int nextOption = (int)this->model.type + 1;
    if (nextOption > 4)
      this->model.type = WorldType::WORLD_TYPE_ORIGINAL;
    else
      this->model.type = static_cast<WorldType>(nextOption);
  }
}

void ScreenNewGame::backToMainMenu() {
  this->context->setScreen(new ScreenMain(this->context));
}

bool ScreenNewGame::canCreateANewWorldWithCurrentName() {
  std::string tempSaveFileName =
      FileUtils::fromCwd("saves/" + inputWorldName + ".tcw");

  return !SaveManager::CheckIfSaveExist(tempSaveFileName.c_str());
}

void ScreenNewGame::createNewWorld() {
  model.name = inputWorldName;
  model.seed = std::stoull(inputSeed);
  model.texturePack = selectedTexturePack->path;
  context->loadGame(model);
}

void ScreenNewGame::renderSelectedOptions() {
  isEditingWorldName ? t_renderer->renderer2D.render(&slotWorldNameInput)
                     : t_renderer->renderer2D.render(&slotWorldName);

  isEditingSeed ? t_renderer->renderer2D.render(&slotSeedInput)
                : t_renderer->renderer2D.render(&slotSeed);

  t_renderer->renderer2D.render(&slotCreateNewWorld);
  t_renderer->renderer2D.render(&slotWorldType);

  if (activeOption == ScreenNewGameOptions::TexturePack)
    t_renderer->renderer2D.render(&slotTextureActive);
  else if (activeOption == ScreenNewGameOptions::WorldName &&
           !isEditingWorldName)
    t_renderer->renderer2D.render(&slotWorldNameActive);
  else if (activeOption == ScreenNewGameOptions::Seed && !isEditingSeed)
    t_renderer->renderer2D.render(&slotSeedActive);
  else if (activeOption == ScreenNewGameOptions::WorldType)
    t_renderer->renderer2D.render(&slotWorldTypeActive);
  else if (activeOption == ScreenNewGameOptions::CreateNewWorld)
    t_renderer->renderer2D.render(&slotCreateNewWorldActive);
}

void ScreenNewGame::saveSeed() {
  inputSeed = std::string(tempSeed.c_str());
  isEditingSeed = false;
}

void ScreenNewGame::cancelEditingSeed() {
  tempSeed = "";
  isEditingSeed = false;
}

void ScreenNewGame::startEditingSeed() {
  tempSeed = std::string(inputSeed.c_str());
  editingIndex = tempSeed.length() - 1;
  updateTempSeedMask();
  isEditingSeed = true;
}

void ScreenNewGame::updateTempSeedMask() {
  tempSeedMask = std::string(tempSeed.c_str());
  tempSeedMask[editingIndex] = '_';
}

void ScreenNewGame::saveWorldName() {
  inputWorldName = Utils::trim(tempWorldName);
  isEditingWorldName = false;
}

void ScreenNewGame::removeWorldNameLastChar() {
  if (tempWorldName.size() > MIN_WORLD_NAME_LENGTH) tempWorldName.pop_back();
  updateTempWorldNameMaskCursor();
}

void ScreenNewGame::addWorldNameLastChar() {
  tempNewChar = tempWorldName.at(tempWorldName.size() - 1);
  if (tempWorldName.size() < MAX_WORLD_NAME_LENGTH)
    tempWorldName.push_back(tempNewChar);
  updateTempWorldNameMaskCursor();
}

void ScreenNewGame::updateTempWorldNameMaskCursor() {
  editingIndexWorldName = tempWorldName.length() - 1;
}

void ScreenNewGame::cancelEditingWorldName() {
  tempWorldName = "";
  isEditingWorldName = false;
}

void ScreenNewGame::startEditingWorldName() {
  tempWorldName = std::string(inputWorldName.c_str());
  updateTempWorldNameMaskCursor();
  updateTempWorldNameMask();
  isEditingWorldName = true;
}

void ScreenNewGame::updateTempWorldNameMask() {
  tempWorldNameMask = std::string(tempWorldName.c_str());
  tempWorldNameMask[editingIndexWorldName] = '_';
}

std::string ScreenNewGame::getSeed() {
  std::string result = "";

  // Rnadom over max int number 4294967295
  result.append(std::to_string(Math::randomi(0, 4)));
  result.append(std::to_string(Math::randomi(0, 2)));
  result.append(std::to_string(Math::randomi(0, 9)));
  result.append(std::to_string(Math::randomi(0, 4)));
  result.append(std::to_string(Math::randomi(0, 9)));
  result.append(std::to_string(Math::randomi(0, 6)));
  result.append(std::to_string(Math::randomi(0, 7)));
  result.append(std::to_string(Math::randomi(0, 2)));
  result.append(std::to_string(Math::randomi(0, 9)));
  result.append(std::to_string(Math::randomi(0, 5)));

  return result;
}

void ScreenNewGame::getAvailableTexturePacks() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  std::string pathPrefix = "textures/texture_packs/";
  std::vector<UtilDirectory> texturePacksFolders =
      Utils::listDir(FileUtils::fromCwd(pathPrefix));
  u8 tempId = 1;

  for (size_t i = 0; i < texturePacksFolders.size(); i++) {
    const UtilDirectory dir = texturePacksFolders.at(i);
    if (dir.isDir) {
      std::string textureDir = pathPrefix + dir.name;
      std::string texturePackInfoPath = textureDir + "/info.json";
      std::ifstream texPackInfo(FileUtils::fromCwd(texturePackInfoPath));

      if (texPackInfo.is_open()) {
        TexturePackInfoModel* model = new TexturePackInfoModel();
        json data = json::parse(texPackInfo);

        model->id = tempId++;
        model->path = std::string(dir.name);
        model->author = data["author"].get<std::string>();
        model->title = data["title"].get<std::string>();
        model->description = data["description"].get<std::string>();

        model->icon.size.set(48, 48);
        model->icon.position.set(135, 158);
        model->icon.mode = Tyra::SpriteMode::MODE_STRETCH;

        textureRepo->add(FileUtils::fromCwd(textureDir + "/icon.png"))
            ->addLink(model->icon.id);

        texturePacks.push_back(model);

        if (dir.name.compare("default") == 0) {
          TYRA_LOG("Setting  selectedTexturePack...");
          selectedTexturePack = model;
        }

        texPackInfo.close();
      }
    }
  }

  if (!selectedTexturePack && texturePacks.size())
    selectedTexturePack = texturePacks[0];
}

void ScreenNewGame::selectPreviousTexturePack() {
  u16 idx = 0;
  for (size_t i = 0; i < texturePacks.size(); i++)
    if (texturePacks.at(i)->id == selectedTexturePack->id) idx = i;

  if (idx == 0)
    selectedTexturePack = texturePacks.at(texturePacks.size() - 1);
  else
    selectedTexturePack = texturePacks.at(idx - 1);
}

void ScreenNewGame::selectNextTexturePack() {
  u16 idx = 0;
  for (size_t i = 0; i < texturePacks.size(); i++)
    if (texturePacks.at(i)->id == selectedTexturePack->id) idx = i;

  if (idx == texturePacks.size() - 1)
    selectedTexturePack = texturePacks.at(0);
  else
    selectedTexturePack = texturePacks.at(idx + 1);
}

void ScreenNewGame::renderWorldNameDialog() {
  t_renderer->renderer2D.render(overlay);
  t_renderer->renderer2D.render(dialogWindow);

  FontOptions titleOptions = FontOptions();
  titleOptions.position = Vec2(246, 135);
  titleOptions.scale = 0.9F;
  titleOptions.alignment = TextAlignment::Center;
  FontManager_printText("Ops!", titleOptions);

  FontOptions dialogueOptions = FontOptions();
  dialogueOptions.position = Vec2(246, 180);
  dialogueOptions.scale = 0.6F;
  dialogueOptions.alignment = TextAlignment::Center;
  FontManager_printText("A world with the same", dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText(" name already exists", dialogueOptions);
  dialogueOptions.position.y += 15;
  FontManager_printText("Please change it", dialogueOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Confirm", 40, 407);
}
