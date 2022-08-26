#include "managers/state_manager.hpp"
#include "renderer/models/color.hpp"
#include "file/file_utils.hpp"
#include <renderer/core/2d/sprite/sprite.hpp>
#include <thread>
#include <chrono>

using Tyra::Audio;
using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;

Context::Context(Engine* t_engine) {
  this->t_renderer = &t_engine->renderer;
  this->t_audio = &t_engine->audio;
  this->t_pad = &t_engine->pad;
}

Context::~Context() {}

void Context::update(const float& deltaTime, Camera* t_camera) {
  this->state->update();
  this->state->render();

  // // Splash Screen
  // if (_state == SPLASH_SCREEN) {
  //   splashScreen->render();
  //   if (splashScreen->shouldBeDestroyed()) {
  //     delete this->splashScreen;
  //     this->splashScreen = NULL;
  //     this->mainMenu = new MainMenu();
  //     this->mainMenu->init(t_renderer, t_audio);
  //     this->_state = MAIN_MENU;
  //   }
  //   return;
  // } else if (_state == MAIN_MENU) {
  //   // Main menu
  //   t_camera->update(*t_pad, *mainMenu->menuSkybox);
  //   mainMenu->update(*t_pad);
  //   mainMenu->render();
  //   if (mainMenu->shouldInitGame()) {
  //     delete mainMenu;
  //     mainMenu = NULL;
  //     loadingScreen->init(t_renderer);
  //     _state = LOADING_SCREEN;
  //   }
  //   return;
  // } else if (_state == LOADING_SCREEN) {
  //   // Main menu
  //   if (loadingScreen->hasFinished()) {
  //     printf("Unloading Loading Screen\n");
  //     loadingScreen->unload();
  //     _state = IN_GAME;
  //   } else {
  //     printf("Loading game\n");
  //     this->loadGame();
  //     printf("Rendering Loading Screen\n");
  //     loadingScreen->render();
  //   }
  //   return;
  // } else if (_state == IN_GAME) {
  //   // In game updates
  //   {
  //     this->controlGameMode(*t_pad);
  //     world->update(player, t_camera, t_pad, deltaTime);
  //     player->update(deltaTime, *t_pad, *t_camera, world->getLoadedBlocks());
  //     ui->update();
  //     t_camera->update(*t_pad, *player->mesh);
  //   }

  //   // Render
  //   {
  //     world->render();
  //     // TODO: Should render only if is third person Cam;
  //     // t_renderer.draw(player->mesh);
  //     ui->render();
  //   }
  // }
}

void Context::loadGame() {
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  if (this->shouldCreatedEntities) {
    printf("Creating entities\n");
    this->world = new World();
    this->player = new Player(t_renderer, t_audio);
    this->itemRepository = new ItemRepository();
    this->ui = new Ui();

    this->loadingScreen->setPercent(25.0F);
    this->shouldCreatedEntities = 0;
    return;
  } else if (this->shouldInitItemRepository) {
    printf("Initing item repo\n");
    this->itemRepository->init(t_renderer);
    this->loadingScreen->setPercent(35.0F);
    this->shouldInitItemRepository = 0;
    return;
  } else if (this->shouldInitUI) {
    printf("Initing UI\n");
    this->ui->init(t_renderer, this->itemRepository, this->player);
    this->loadingScreen->setPercent(50.0F);
    this->shouldInitUI = 0;
    return;
  } else if (this->shouldInitWorld) {
    printf("Initing world\n");
    this->world->init(t_renderer, this->itemRepository);
    this->loadingScreen->setPercent(90.0F);
    this->shouldInitWorld = 0;
    return;
  } else if (this->shouldInitPlayer) {
    printf("Initing player\n");
    this->player->mesh->getPosition()->set(world->getGlobalSpawnArea());
    this->player->spawnArea.set(world->getLocalSpawnArea());
    this->loadingScreen->setPercent(100.0F);
    this->loadingScreen->setState(LoadingState::Complete);
    this->shouldInitPlayer = 0;
    return;
  }

  printf("\nGAME LOADED\n");
}

// TODO : Check the delay to change
void Context::controlGameMode(Pad& t_pad) {
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
