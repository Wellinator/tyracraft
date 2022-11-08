#include "states/game_play/states/creative/creative_playing_state.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->t_fontManager =
      new FontManager(&t_context->context->t_engine->renderer);
}

CreativePlayingState::~CreativePlayingState() {
  this->stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
  delete this->t_creativeAudioListener;
  delete this->t_fontManager;
}

void CreativePlayingState::init() {
  this->t_creativeAudioListener = new CreativeAudioListener(
      &this->stateGamePlay->context->t_engine->audio.song);
  this->t_creativeAudioListener->playRandomCreativeSound();
  this->audioListenerId =
      this->stateGamePlay->context->t_engine->audio.song.addListener(
          this->t_creativeAudioListener);
}

void CreativePlayingState::update(const float& deltaTime) {
  elapsedTimeInSec += deltaTime;
  this->handleInput();

  Threading::switchThread();
  this->stateGamePlay->world->update(
      this->stateGamePlay->player, this->stateGamePlay->context->t_camera,
      &this->stateGamePlay->context->t_engine->pad, deltaTime);

  Threading::switchThread();
  this->stateGamePlay->player->update(
      deltaTime, this->stateGamePlay->context->t_engine->pad,
      *this->stateGamePlay->context->t_camera,
      this->stateGamePlay->world->getLoadedBlocks());

  Threading::switchThread();
  this->stateGamePlay->ui->update();

  Threading::switchThread();
  this->stateGamePlay->context->t_camera->update(
      this->stateGamePlay->context->t_engine->pad,
      *this->stateGamePlay->player->mesh);
}

void CreativePlayingState::render() {
  this->stateGamePlay->world->render();

  Threading::switchThread();
  this->stateGamePlay->player->render();

  Threading::switchThread();
  this->renderCreativeUi();

  if (debugMode) drawDegubInfo();
}

void CreativePlayingState::handleInput() {
  const auto& clicked =
      this->stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.Cross) {
    if (elapsedTimeInSec < 0.5F) {
      this->stateGamePlay->player->toggleFlying();
    }
    elapsedTimeInSec = 0.0F;
  }

  if (clicked.Select) debugMode = !debugMode;
}

void CreativePlayingState::navigate() {}

void CreativePlayingState::renderCreativeUi() {
  this->stateGamePlay->ui->renderCrosshair();
  this->stateGamePlay->ui->renderInventory();
}

void CreativePlayingState::drawDegubInfo() {
  Info* info = &this->stateGamePlay->context->t_engine->info;

  // Draw seed
  std::string seed = std::string("Seed: ").append(
      std::to_string(this->stateGamePlay->world->getSeed()));
  this->t_fontManager->printText(seed, FontOptions(Vec2(5.0f, 5.0f)));

  // Draw FPS:
  std::string fps = std::string("FPS: ").append(std::to_string(info->getFps()));
  this->t_fontManager->printText(fps, FontOptions(Vec2(5.0f, 25.0f)));

  // Draw Free RAM:
  // std::string freeRam = std::string("Free RAM: ")
  //                           .append(std::to_string(info->getAvailableRAM()))
  //                           .append(" MB");
  // this->t_fontManager->printText(freeRam.c_str(), 5.0f, 32.0f);
}
