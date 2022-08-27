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

Context::Context(Engine* t_engine, Camera* t_camera) {
  this->t_renderer = &t_engine->renderer;
  this->t_audio = &t_engine->audio;
  this->t_pad = &t_engine->pad;
  this->t_camera = t_camera;
}

Context::~Context() { delete this->state; }

void Context::update(const float& deltaTime) {
  this->state->update(deltaTime);
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

void Context::setState(GameState* newState) {
  if (this->state != nullptr) delete this->state;
  this->state = newState;
}
