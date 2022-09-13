#include "states/game_play/state_game_play.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"

using Tyra::Audio;
using Tyra::DynamicPipeline;
using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::RendererSettings;

StateGamePlay::StateGamePlay(Context* t_context) : GameState(t_context) {
  this->init();
}

StateGamePlay::~StateGamePlay() {
  this->context->t_audio->song.stop();
  this->context->t_audio->song.inLoop = false;
  delete this->world;
  delete this->ui;
  delete this->player;
  delete this->itemRepository;
}

void StateGamePlay::init() {
  // TODO: add in game skybox;
  this->dynpip.setRenderer(&this->context->t_renderer->core);
}

void StateGamePlay::update(const float& deltaTime) {
  this->handleInput();
  this->world->update(this->player, this->context->t_camera,
                      this->context->t_pad, deltaTime);
  this->player->update(deltaTime, *this->context->t_pad,
                       *this->context->t_camera,
                       this->world->getLoadedBlocks());
  this->ui->update();
  this->context->t_camera->update(*this->context->t_pad, *this->player->mesh);
}

void StateGamePlay::render() {
  this->world->render();

  {
    // TODO: Should render only if is third person Cam;
    TYRA_LOG("Rendering Player");
    this->context->t_renderer->renderer3D.usePipeline(&dynpip);
    dynpip.render(this->player->mesh.get());
  }

  this->ui->render();
}

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
