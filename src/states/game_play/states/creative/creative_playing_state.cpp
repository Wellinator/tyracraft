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
  this->t_fontManager->printText(
      seed, FontOptions(Vec2(5.0f, 5.0f), Color(255), 0.8F));

  // Draw FPS:
  std::string fps = std::string("FPS: ").append(std::to_string(info->getFps()));
  this->t_fontManager->printText(
      fps, FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.8F));

  // Draw Free RAM:
  // std::string freeRam = std::string("Free RAM: ")
  //                           .append(std::to_string(info->getAvailableRAM()))
  //                           .append(" MB");
  // this->t_fontManager->printText(freeRam.c_str(), 5.0f, 32.0f);

  // Draw blocks info:
  // float offsetY = 30.0f;
  // for (size_t i = (u8)Blocks::STONE_BLOCK; i < (u8)Blocks::TOTAL_OF_BLOCKS;
  //      i++) {
  //   BlockInfo info =
  //       *this->stateGamePlay->world->blockManager->getBlockTexOffsetByType(
  //           static_cast<Blocks>(i));

  //   std::string blocksInfo = std::string("\nID: ")
  //                                .append(std::to_string(i))
  //                                .append(std::string(" | X: "))
  //                                .append(std::to_string(info._texOffssetX))
  //                                .append(std::string(" | Y: "))
  //                                .append(std::to_string(info._texOffssetX));

  //   this->t_fontManager->printText(
  //       blocksInfo, FontOptions(Vec2(5.0f, offsetY), Color(255), 0.8F));
  //   offsetY += 15.0F;
  // }

  // Draw Player state:
  std::string playerStateInfo =
      std::string("Player state\n")
          .append(std::string("Is On ground: ") +
                  std::to_string(this->stateGamePlay->player->isOnGround) +
                  std::string("\n"))
          .append(std::string("Is Flaying: ") +
                  std::to_string(this->stateGamePlay->player->isFlying) +
                  std::string("\n"))
          .append(std::string("Is Breaking: ") +
                  std::to_string(this->stateGamePlay->player->isBreaking) +
                  std::string("\n"));

  this->t_fontManager->printText(
      playerStateInfo, FontOptions(Vec2(310.0f, 5.0f), Color(255), 0.8F));
}
