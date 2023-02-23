#include "states/main_menu/screens/screen_new_game.hpp"
#include "states/main_menu/screens/screen_main.hpp"

ScreenNewGame::ScreenNewGame(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = &t_context->context->t_engine->renderer;
  this->inputSeed = getSeed();
  this->init();
}

ScreenNewGame::~ScreenNewGame() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  textureRepo->free(slotTexture->id);
  textureRepo->free(slotActiveTexture->id);

  textureRepo->freeBySprite(backgroundNewGame);
  textureRepo->freeBySprite(slotSeedInput);
  textureRepo->freeBySprite(slotTextureActive);

  textureRepo->freeBySprite(btnTriangle);
  textureRepo->freeBySprite(btnCross);
  textureRepo->freeBySprite(btnCircle);

  for (size_t i = 0; i < texturePacks.size(); i++) {
    textureRepo->freeBySprite(texturePacks.at(i)->icon);
    delete texturePacks.at(i);
  }
}

void ScreenNewGame::update() {
  fpsCounter++;
  this->handleInput();
}

void ScreenNewGame::render() {
  this->t_renderer->renderer2D.render(&backgroundNewGame);

  this->renderSelectedOptions();

  Color infoColor = Color(70, 70, 70);
  FontManager_printText("NEW WORLD",
                        FontOptions(Vec2(190, 140), infoColor, 0.8F));
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

  if (isEditingSeed) {
    FontOptions options;
    options.position = Vec2(175, 232);
    options.color = Color(168, 160, 50);

    if (fpsCounter < 128) {
      FontManager_printText(this->tempSeed.c_str(), options);
    } else {
      FontManager_printText(this->tempSeedMask.c_str(), options);
    }

  } else {
    FontManager_printText(std::string("Seed: ").append(this->inputSeed).c_str(),
                          140, 232);
  }

  switch (this->model.type) {
    case WorldType::WORLD_TYPE_ORIGINAL:
      FontManager_printText("World Type: Original", 130, 277);
      break;
    case WorldType::WORLD_TYPE_FLAT:
      FontManager_printText("World Type: Flat", 150, 277);
      break;
    case WorldType::WORLD_TYPE_ISLAND:
      FontManager_printText("World Type: Island", 135, 277);
      break;
    case WorldType::WORLD_TYPE_WOODS:
      FontManager_printText("World Type: Woods", 140, 277);
      break;
    case WorldType::WORLD_TYPE_FLOATING:
      FontManager_printText("World Type: Floating", 130, 277);
      break;

    default:
      break;
  }

  FontManager_printText("Create New World", 145, 322);

  if (isEditingSeed) {
    this->t_renderer->renderer2D.render(&btnTriangle);
    this->t_renderer->renderer2D.render(&btnCross);

    FontManager_printText("Confirm", 35, 407);
    FontManager_printText("Cancel", 160, 407);
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
      } else {
        FontManager_printText("Select", 35, 407);
      }
      FontManager_printText("Back", 160, 407);
    }
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
      ->add(FileUtils::fromCwd("textures/gui/menu/background_new_game.png"))
      ->addLink(backgroundNewGame.id);

  slotSeed.mode = Tyra::MODE_STRETCH;
  slotSeed.size.set(256, 32);
  slotSeed.position.set(125, 230);
  slotTexture->addLink(slotSeed.id);

  slotSeedActive.mode = Tyra::MODE_STRETCH;
  slotSeedActive.size.set(256, 32);
  slotSeedActive.position.set(125, 230);
  slotActiveTexture->addLink(slotSeedActive.id);

  slotSeedInput.mode = Tyra::MODE_STRETCH;
  slotSeedInput.size.set(256, 32);
  slotSeedInput.position.set(125, 230);
  textureRepo->add(FileUtils::fromCwd("textures/gui/slot_input.png"))
      ->addLink(slotSeedInput.id);

  slotTextureActive.mode = Tyra::MODE_STRETCH;
  slotTextureActive.size.set(68, 68);
  slotTextureActive.position.set(125, 144);
  textureRepo
      ->add(FileUtils::fromCwd("textures/gui/slot_texture_pack_active.png"))
      ->addLink(slotTextureActive.id);

  slotWorldType.mode = Tyra::MODE_STRETCH;
  slotWorldType.size.set(256, 32);
  slotWorldType.position.set(125, 275);
  slotTexture->addLink(slotWorldType.id);

  slotWorldTypeActive.mode = Tyra::MODE_STRETCH;
  slotWorldTypeActive.size.set(256, 32);
  slotWorldTypeActive.position.set(125, 275);
  slotActiveTexture->addLink(slotWorldTypeActive.id);

  slotCreateNewWorld.mode = Tyra::MODE_STRETCH;
  slotCreateNewWorld.size.set(256, 32);
  slotCreateNewWorld.position.set(125, 320);
  slotTexture->addLink(slotCreateNewWorld.id);

  slotCreateNewWorldActive.mode = Tyra::MODE_STRETCH;
  slotCreateNewWorldActive.size.set(256, 32);
  slotCreateNewWorldActive.position.set(125, 320);
  slotActiveTexture->addLink(slotCreateNewWorldActive.id);

  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15,
                        this->t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(
      140, this->t_renderer->core.getSettings().getHeight() - 40);

  btnTriangleTexturePack.mode = Tyra::MODE_STRETCH;
  btnTriangleTexturePack.size.set(25, 25);
  btnTriangleTexturePack.position.set(
      260, this->t_renderer->core.getSettings().getHeight() - 40);

  Texture* btnTriangleTexture =
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
}

void ScreenNewGame::handleInput() {
  if (isEditingSeed) {
    handleSeedInput();
  } else {
    handleOptionsSelection();
  }
}

void ScreenNewGame::handleOptionsSelection() {
  auto clickedButtons = this->context->context->t_engine->pad.getClicked();

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
    if (this->selectedOption == ScreenNewGameOptions::CreateNewWorld)
      return this->createNewWorld();
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

void ScreenNewGame::createNewWorld() {
  model.seed = std::stoull(inputSeed);
  model.texturePack = selectedTexturePack->path;
  context->loadGame(model);
}

void ScreenNewGame::renderSelectedOptions() {
  isEditingSeed ? this->t_renderer->renderer2D.render(&slotSeedInput)
                : this->t_renderer->renderer2D.render(&slotSeed);

  this->t_renderer->renderer2D.render(&slotCreateNewWorld);
  this->t_renderer->renderer2D.render(&slotWorldType);

  if (this->activeOption == ScreenNewGameOptions::TexturePack)
    this->t_renderer->renderer2D.render(&slotTextureActive);
  else if (this->activeOption == ScreenNewGameOptions::Seed && !isEditingSeed)
    this->t_renderer->renderer2D.render(&slotSeedActive);
  else if (this->activeOption == ScreenNewGameOptions::WorldType)
    this->t_renderer->renderer2D.render(&slotWorldTypeActive);
  else if (this->activeOption == ScreenNewGameOptions::CreateNewWorld)
    this->t_renderer->renderer2D.render(&slotCreateNewWorldActive);
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

        model->icon.size.set(64, 64);
        model->icon.position.set(127, 146);
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
