#pragma once
#include "states/main_menu/screens/screen_new_game.hpp"
#include "states/main_menu/screens/screen_main.hpp"

ScreenNewGame::ScreenNewGame(StateMainMenu* t_context) : ScreenBase(t_context) {
  this->t_renderer = t_context->context->t_renderer;
  this->init();
}

ScreenNewGame::~ScreenNewGame() {
  this->t_renderer->getTextureRepository().free(
      checkboxUnfilledWhiteBorderTexture->id);
  this->t_renderer->getTextureRepository().free(
      checkboxFilledWhiteBorderTexture->id);
  this->t_renderer->getTextureRepository().freeBySprite(backgroundNewGame);
  this->t_renderer->getTextureRepository().freeBySprite(textFlatWorld);
  this->t_renderer->getTextureRepository().freeBySprite(textEnableTrees);
  this->t_renderer->getTextureRepository().freeBySprite(textEnableWater);
  this->t_renderer->getTextureRepository().freeBySprite(textCreateNewWorld);
  this->t_renderer->getTextureRepository().freeBySprite(slotCreateNewWorld);
  this->t_renderer->getTextureRepository().freeBySprite(textBack);
  this->t_renderer->getTextureRepository().freeBySprite(textSelect);
  this->t_renderer->getTextureRepository().freeBySprite(btnTriangle);
  this->t_renderer->getTextureRepository().freeBySprite(btnCross);
  this->t_renderer->getTextureRepository().freeBySprite(
      slotCreateNewWorldActive);
}

void ScreenNewGame::update() {
  this->handleInput();
  this->hightLightActiveOption();
}

void ScreenNewGame::render() {
  this->t_renderer->renderer2D.render(&backgroundNewGame);
  this->t_renderer->renderer2D.render(&checkboxUnfilledFlatWorld);
  this->t_renderer->renderer2D.render(&checkboxUnfilledEnableTrees);
  this->t_renderer->renderer2D.render(&checkboxUnfilledEnableWater);
  this->renderSelectedOptions();

  this->t_renderer->renderer2D.render(&textFlatWorld);
  this->t_renderer->renderer2D.render(&textEnableTrees);
  this->t_renderer->renderer2D.render(&textEnableWater);
  this->t_renderer->renderer2D.render(&textCreateNewWorld);

  this->t_renderer->renderer2D.render(&textBack);
  this->t_renderer->renderer2D.render(&textSelect);
  this->t_renderer->renderer2D.render(&btnTriangle);
  this->t_renderer->renderer2D.render(&btnCross);
}

void ScreenNewGame::init() {
  backgroundNewGame.mode = Tyra::MODE_STRETCH;
  backgroundNewGame.size.set(512, 512);
  backgroundNewGame.position.set(0, 0);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/background_new_game.png"))
      ->addLink(backgroundNewGame.id);

  // Checkbox
  {
    checkboxUnfilledFlatWorld.mode = Tyra::MODE_STRETCH;
    checkboxUnfilledFlatWorld.size.set(16, 16);
    checkboxUnfilledFlatWorld.position.set(132, 214);

    checkboxUnfilledEnableTrees.mode = Tyra::MODE_STRETCH;
    checkboxUnfilledEnableTrees.size.set(16, 16);
    checkboxUnfilledEnableTrees.position.set(132, 214 + 30);

    checkboxUnfilledEnableWater.mode = Tyra::MODE_STRETCH;
    checkboxUnfilledEnableWater.size.set(16, 16);
    checkboxUnfilledEnableWater.position.set(132, 214 + 60);

    checkboxUnfilledWhiteBorderTexture =
        this->t_renderer->getTextureRepository().add(FileUtils::fromCwd(
            "assets/textures/ui/checkbox_unfilled_white_border.png"));

    checkboxUnfilledWhiteBorderTexture->addLink(checkboxUnfilledFlatWorld.id);
    checkboxUnfilledWhiteBorderTexture->addLink(checkboxUnfilledEnableTrees.id);
    checkboxUnfilledWhiteBorderTexture->addLink(checkboxUnfilledEnableWater.id);

    checkboxFilledFlatWorld.mode = Tyra::MODE_STRETCH;
    checkboxFilledFlatWorld.size.set(16, 16);
    checkboxFilledFlatWorld.position.set(132, 214);

    checkboxFilledEnableTrees.mode = Tyra::MODE_STRETCH;
    checkboxFilledEnableTrees.size.set(16, 16);
    checkboxFilledEnableTrees.position.set(132, 214 + 30);

    checkboxFilledEnableWater.mode = Tyra::MODE_STRETCH;
    checkboxFilledEnableWater.size.set(16, 16);
    checkboxFilledEnableWater.position.set(132, 214 + 60);

    checkboxFilledWhiteBorderTexture =
        this->t_renderer->getTextureRepository().add(FileUtils::fromCwd(
            "assets/textures/ui/checkbox_filled_white_border.png"));

    checkboxFilledWhiteBorderTexture->addLink(checkboxFilledFlatWorld.id);
    checkboxFilledWhiteBorderTexture->addLink(checkboxFilledEnableTrees.id);
    checkboxFilledWhiteBorderTexture->addLink(checkboxFilledEnableWater.id);
  }

  // Checkbox text options
  {
    textFlatWorld.mode = Tyra::MODE_STRETCH;
    textFlatWorld.size.set(128, 16);
    textFlatWorld.position.set(132 + 20, 214);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/text_flat_world.png"))
        ->addLink(textFlatWorld.id);

    textEnableTrees.mode = Tyra::MODE_STRETCH;
    textEnableTrees.size.set(128, 16);
    textEnableTrees.position.set(132 + 20, 214 + 30);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/text_enable_trees.png"))
        ->addLink(textEnableTrees.id);

    textEnableWater.mode = Tyra::MODE_STRETCH;
    textEnableWater.size.set(256, 32);
    textEnableWater.position.set(132 + 20, 214 + 60);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/text_enable_water.png"))
        ->addLink(textEnableWater.id);
  }

  textCreateNewWorld.mode = Tyra::MODE_STRETCH;
  textCreateNewWorld.size.set(256, 34);
  textCreateNewWorld.position.set(120, 327);

  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("assets/menu/text_create_new_world.png"))
      ->addLink(textCreateNewWorld.id);

  // Slot
  {
    slotCreateNewWorld.mode = Tyra::MODE_STRETCH;
    slotCreateNewWorld.size.set(256, 32);
    slotCreateNewWorld.position.set(125, 320);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/slot.png"))
        ->addLink(slotCreateNewWorld.id);

    slotCreateNewWorldActive.mode = Tyra::MODE_STRETCH;
    slotCreateNewWorldActive.size.set(256, 32);
    slotCreateNewWorldActive.position.set(125, 320);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/slot_active.png"))
        ->addLink(slotCreateNewWorldActive.id);
  }

  // Buttons
  {
    btnCross.mode = Tyra::MODE_STRETCH;
    btnCross.size.set(25, 25);
    btnCross.position.set(
        15, this->t_renderer->core.getSettings().getHeight() - 40);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/textures/ui/btn_cross.png"))
        ->addLink(btnCross.id);

    textSelect.mode = Tyra::MODE_STRETCH;
    textSelect.size.set(64, 15);
    textSelect.position.set(
        15 + 30, this->t_renderer->core.getSettings().getHeight() - 36);
    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/text_select.png"))
        ->addLink(textSelect.id);

    btnTriangle.mode = Tyra::MODE_STRETCH;
    btnTriangle.size.set(25, 25);
    btnTriangle.position.set(
        124, this->t_renderer->core.getSettings().getHeight() - 40);

    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/textures/ui/btn_triangle.png"))
        ->addLink(btnTriangle.id);

    textBack.mode = Tyra::MODE_STRETCH;
    textBack.size.set(64, 15);
    textBack.position.set(
        124 + 30, this->t_renderer->core.getSettings().getHeight() - 36);
    this->t_renderer->getTextureRepository()
        .add(FileUtils::fromCwd("assets/menu/text_back.png"))
        ->addLink(textBack.id);
  }
}

void ScreenNewGame::handleInput() {
  // Change active option
  {
    if (this->context->context->t_pad->getClicked().DpadDown) {
      this->playClickSound();
      int nextOption = (int)this->activeOption + 1;
      if (nextOption > 3)
        this->activeOption = ScreenNewGameOptions::FlatWorld;
      else
        this->activeOption = static_cast<ScreenNewGameOptions>(nextOption);
    } else if (this->context->context->t_pad->getClicked().DpadUp) {
      this->playClickSound();
      int nextOption = (int)this->activeOption - 1;
      if (nextOption < 0)
        this->activeOption = ScreenNewGameOptions::CreateNewWorld;
      else
        this->activeOption = static_cast<ScreenNewGameOptions>(nextOption);
    }
  }

  if (this->context->context->t_pad->getClicked().Triangle) {
    this->playClickSound();
    this->backToMainMenu();
  } else if (this->context->context->t_pad->getClicked().Cross) {
    this->playClickSound();
    this->selectedOption = this->activeOption;
    this->updateModel();
    if (this->selectedOption == ScreenNewGameOptions::CreateNewWorld)
      return this->createNewWorld();
  }
}

void ScreenNewGame::updateModel() {
  if (this->selectedOption == ScreenNewGameOptions::FlatWorld)
    this->model.makeFlat = !this->model.makeFlat;
  else if (this->selectedOption == ScreenNewGameOptions::EnableTrees)
    this->model.enableTrees = !this->model.enableTrees;
  else if (this->selectedOption == ScreenNewGameOptions::EnableWater)
    this->model.enableWater = !this->model.enableWater;
}

void ScreenNewGame::backToMainMenu() {
  this->context->setScreen(new ScreenMain(this->context));
}

void ScreenNewGame::createNewWorld() { this->context->loadGame(this->model); }

void ScreenNewGame::renderSelectedOptions() {
  if (this->model.makeFlat)
    this->t_renderer->renderer2D.render(&checkboxFilledFlatWorld);
  if (this->model.enableTrees)
    this->t_renderer->renderer2D.render(&checkboxFilledEnableTrees);
  if (this->model.enableWater)
    this->t_renderer->renderer2D.render(&checkboxFilledEnableWater);

  if (this->activeOption == ScreenNewGameOptions::CreateNewWorld)
    this->t_renderer->renderer2D.render(&slotCreateNewWorldActive);
  else
    this->t_renderer->renderer2D.render(&slotCreateNewWorld);
}

void ScreenNewGame::hightLightActiveOption() {
  // Reset sprite colors
  {
    this->textFlatWorld.color = Tyra::Color(128, 128, 128);
    this->textEnableTrees.color = Tyra::Color(128, 128, 128);
    this->textEnableWater.color = Tyra::Color(128, 128, 128);
    this->textCreateNewWorld.color = Tyra::Color(128, 128, 128);
  }

  Sprite* t_selectedOptionSprite = NULL;
  if (this->activeOption == ScreenNewGameOptions::FlatWorld)
    t_selectedOptionSprite = &this->textFlatWorld;
  else if (this->activeOption == ScreenNewGameOptions::EnableTrees)
    t_selectedOptionSprite = &this->textEnableTrees;
  else if (this->activeOption == ScreenNewGameOptions::EnableWater)
    t_selectedOptionSprite = &this->textEnableWater;
  else if (this->activeOption == ScreenNewGameOptions::CreateNewWorld)
    t_selectedOptionSprite = &this->textCreateNewWorld;

  t_selectedOptionSprite->color = Tyra::Color(230, 230, 0);
}

void ScreenNewGame::playClickSound() {
  this->context->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                                  SoundFX::Click);
}
