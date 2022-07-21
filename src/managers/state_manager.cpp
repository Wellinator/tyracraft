#include "managers/state_manager.hpp"
#include "renderer/models/color.hpp"

using Tyra::Audio;
using Tyra::Color;
using Tyra::Renderer;

StateManager::StateManager() {}

StateManager::~StateManager() {}

void StateManager::init(Renderer* t_renderer, Audio* t_audio) {
  this->t_renderer = t_renderer;
  this->t_audio = t_audio;

  splashScreen = new SplashScreen(t_renderer);
  mainMenu = new MainMenu();
}

void StateManager::update(float deltaTime, Pad& t_pad, Camera& camera) {
  // Splash Screen
  if (_state == SPLASH_SCREEN) {
    splashScreen->render();
    if (splashScreen->shouldBeDestroyed()) {
      delete splashScreen;
      splashScreen = NULL;
      this->mainMenu->init(t_renderer, t_audio);
      _state = MAIN_MENU;
    }
    return;
  }

  // Main menu
  if (_state == MAIN_MENU) {
    // camera.update(t_pad, mainMenu->menuSkybox);
    mainMenu->update(t_pad);
    mainMenu->render();
    if (mainMenu->shouldInitGame()) {
      delete mainMenu;
      mainMenu = NULL;
      this->loadGame();  // TODO: implement loading screen
      _state = IN_GAME;
    }
    return;
  }

  // In game
  if (_state == IN_GAME) {
    this->controlGameMode(t_pad);

    // Updates
    world->update(player, &camera, &t_pad);
    player->update(deltaTime, t_pad, camera,
                   world->terrainManager->getChunck()->blocks.data(),
                   world->terrainManager->getChunck()->blocks.size());
    ui->update();
    camera.update(t_pad, *player->mesh);

    // Render
    world->render();
    // TODO: Should render only if is third person Cam;
    // t_renderer->draw(player->mesh);

    // Lest spet is 2D drawing
    ui->render();
  }
}

u8 StateManager::currentState() { return this->_state; }

void StateManager::loadSplashScreen() {}

void StateManager::loadMenu() {}

void StateManager::loadInGameMenu() {}

void StateManager::loadGame() {
  world = new World();
  player = new Player(t_renderer, t_audio);
  itemRepository = new ItemRepository();
  ui = new Ui();

  setBgColorAndAmbientColor();

  itemRepository->init(t_renderer);
  ui->init(t_renderer, itemRepository, player);
  world->init(t_renderer, itemRepository);

  player->mesh->getPosition()->set(world->terrainManager->worldSpawnArea);
  player->spawnArea.set(world->terrainManager->worldSpawnArea);
}

void StateManager::play() { loadGame(); }

void StateManager::setBgColorAndAmbientColor() {
  t_renderer->setClearScreenColor(Color(192.0F, 216.0F, 255.0F));
}

// TODO: Check the delay to change
void StateManager::controlGameMode(Pad& t_pad) {
  if (t_pad.getPressed().DpadUp && t_pad.getClicked().Cross) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            this->lastTimeCrossWasClicked - std::chrono::steady_clock::now())
            .count() <= 500) {
      printf("GAME_MODE changed to %d\n", this->gameMode);
      this->gameMode = this->gameMode == CREATIVE ? SURVIVAL : CREATIVE;
    }
    this->lastTimeCrossWasClicked = std::chrono::steady_clock::now();
  }
}
