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

  textureRepo->freeBySprite(btnTriangle);
  textureRepo->freeBySprite(btnCross);
  textureRepo->freeBySprite(btnCircle);
}

void ScreenNewGame::update() {
  fpsCounter++;
  this->handleInput();
}

void ScreenNewGame::render() {
  this->t_renderer->renderer2D.render(&backgroundNewGame);
  this->renderSelectedOptions();

  if (isEditingSeed) {
    FontOptions options;
    options.position = Vec2(175, 232);
    options.color = Color(168, 160, 50);

    if (fpsCounter < 128) {
      this->context->t_fontManager->printText(this->tempSeed.c_str(), options);
    } else {
      this->context->t_fontManager->printText(this->tempSeedMask.c_str(),
                                              options);
    }

  } else {
    this->context->t_fontManager->printText(
        std::string("Seed: ").append(this->inputSeed).c_str(), 140, 232);
  }

  switch (this->model.type) {
    case WorldType::WORLD_TYPE_ORIGINAL:
      this->context->t_fontManager->printText("World Type: Original", 130, 277);
      break;
    case WorldType::WORLD_TYPE_FLAT:
      this->context->t_fontManager->printText("World Type: Flat", 150, 277);
      break;
    case WorldType::WORLD_TYPE_ISLAND:
      this->context->t_fontManager->printText("World Type: Island", 135, 277);
      break;
    case WorldType::WORLD_TYPE_WOODS:
      this->context->t_fontManager->printText("World Type: Woods", 140, 277);
      break;
    case WorldType::WORLD_TYPE_FLOATING:
      this->context->t_fontManager->printText("World Type: Floating", 130, 277);
      break;

    default:
      break;
  }

  this->context->t_fontManager->printText("Create New World", 145, 322);

  this->t_renderer->renderer2D.render(&btnTriangle);
  this->t_renderer->renderer2D.render(&btnCross);

  if (isEditingSeed) {
    this->context->t_fontManager->printText("Confirm", 35, 407);
    this->context->t_fontManager->printText("Cancel", 160, 407);
  } else {
    if (activeOption == ScreenNewGameOptions::Seed) {
      this->context->t_fontManager->printText("Edit", 35, 407);
      this->context->t_fontManager->printText("Random", 280, 407);
      this->t_renderer->renderer2D.render(&btnCircle);
    } else {
      this->context->t_fontManager->printText("Select", 35, 407);
    }
    this->context->t_fontManager->printText("Back", 160, 407);
  }
}

void ScreenNewGame::init() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  slotTexture = textureRepo->add(FileUtils::fromCwd("assets/menu/slot.png"));
  slotActiveTexture =
      textureRepo->add(FileUtils::fromCwd("assets/menu/slot_active.png"));

  backgroundNewGame.mode = Tyra::MODE_STRETCH;
  backgroundNewGame.size.set(512, 512);
  backgroundNewGame.position.set(0, 0);

  textureRepo->add(FileUtils::fromCwd("assets/menu/background_new_game.png"))
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
  textureRepo->add(FileUtils::fromCwd("assets/menu/slot_input.png"))
      ->addLink(slotSeedInput.id);

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

  textureRepo->add(FileUtils::fromCwd("assets/textures/ui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(
      140, this->t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("assets/textures/ui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  btnCircle.mode = Tyra::MODE_STRETCH;
  btnCircle.size.set(25, 25);
  btnCircle.position.set(260,
                         this->t_renderer->core.getSettings().getHeight() - 40);

  textureRepo->add(FileUtils::fromCwd("assets/textures/ui/btn_circle.png"))
      ->addLink(btnCircle.id);
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
    if (nextOption > 2)
      this->activeOption = ScreenNewGameOptions::Seed;
    else
      this->activeOption = static_cast<ScreenNewGameOptions>(nextOption);
  } else if (clickedButtons.DpadUp) {
    int nextOption = (int)this->activeOption - 1;
    if (nextOption < 0)
      this->activeOption = ScreenNewGameOptions::CreateNewWorld;
    else
      this->activeOption = static_cast<ScreenNewGameOptions>(nextOption);
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
  auto validChars = this->context->t_fontManager->NumericValidChars;
  std::size_t found = validChars.find(tempSeed[editingIndex]);
  u8 currentCharIndex = found != std::string::npos ? found : 0;

  if (clickedButtons.DpadDown) {
    u16 newCharIndex = currentCharIndex - 1 < 0 ? validChars.length() - 1
                                                : currentCharIndex - 1;

    tempSeed[editingIndex] = validChars[newCharIndex];
  } else if (clickedButtons.DpadUp) {
    u16 newCharIndex =
        currentCharIndex + 1 >= validChars.length() ? 0 : currentCharIndex + 1;

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
  context->loadGame(model);
}

void ScreenNewGame::renderSelectedOptions() {
  isEditingSeed ? this->t_renderer->renderer2D.render(&slotSeedInput)
                : this->t_renderer->renderer2D.render(&slotSeed);

  this->t_renderer->renderer2D.render(&slotCreateNewWorld);
  this->t_renderer->renderer2D.render(&slotWorldType);

  if (this->activeOption == ScreenNewGameOptions::Seed && !isEditingSeed)
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
  static uint32_t Z;
  if (Z & 1) {
    Z = (Z >> 1);
  } else {
    Z = (Z >> 1) ^ 0x7FFFF159;
  }

  return std::to_string(Z);
}
