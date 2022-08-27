#include "states/game_play/state_game_play.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::RendererSettings;

StateGamePlay::StateGamePlay(Context* t_context) : GameState(t_context) {}

StateGamePlay::~StateGamePlay() {
  this->context->t_audio->song.stop();
  this->context->t_audio->song.inLoop = false;
  this->unload();
}

void StateGamePlay::init() {
  // TODO: add in game skybox;
}

void StateGamePlay::update(const float& deltaTime) {
  printf("UPDATING\n");
  this->handleInput();
  this->context->world->update(this->context->player, this->context->t_camera,
                               this->context->t_pad, deltaTime);
  this->context->player->update(deltaTime, *this->context->t_pad,
                                *this->context->t_camera,
                                this->context->world->getLoadedBlocks());
  this->context->ui->update();
  this->context->t_camera->update(*this->context->t_pad,
                                  *this->context->player->mesh);
}

void StateGamePlay::render() {
  printf("RENDERING\n");
  this->context->world->render();
  // TODO: Should render only if is third person Cam;
  // this->context->t_renderer.draw(player->mesh);
  this->context->ui->render();
}

void StateGamePlay::unload() {}

void StateGamePlay::handleInput() { this->controlGameMode(); }

// TODO : Check the delay to change
void StateGamePlay::controlGameMode() {
  if (this->context->t_pad->getPressed().DpadUp &&
      this->context->t_pad->getClicked().Cross) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(
            this->lastTimeCrossWasClicked - std::chrono::steady_clock::now())
            .count() <= 500) {
      printf("GAME_MODE changed to %d\n", (u8)this->gameMode);
      this->gameMode = this->gameMode == GameMode::Creative
                           ? GameMode::Survival
                           : GameMode::Creative;
    }
    this->lastTimeCrossWasClicked = std::chrono::steady_clock::now();
  }
}
