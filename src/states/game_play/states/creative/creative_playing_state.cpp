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
  this->audioListenerId =
      this->stateGamePlay->context->t_engine->audio.song.addListener(
          this->t_creativeAudioListener);
  this->t_creativeAudioListener->playRandomCreativeSound();
}

void CreativePlayingState::update(const float& deltaTime) {
  elapsedTimeInSec += deltaTime;
  tickManager.update(std::min(deltaTime, MAX_FRAME_MS));
  this->handleInput();

  this->stateGamePlay->world->update(
      this->stateGamePlay->player, this->stateGamePlay->context->t_camera,
      &this->stateGamePlay->context->t_engine->pad, deltaTime);

  this->stateGamePlay->player->update(
      deltaTime, this->stateGamePlay->context->t_engine->pad,
      *this->stateGamePlay->context->t_camera,
      this->stateGamePlay->world->chunckManager->getVisibleChunks());

  this->stateGamePlay->ui->update();

  this->stateGamePlay->context->t_camera->update(
      this->stateGamePlay->context->t_engine->pad,
      *this->stateGamePlay->player->mesh);

  if (!isSongPlaying()) playNewRandomSong();

  Threading::switchThread();
}

void CreativePlayingState::render() {
  this->stateGamePlay->world->render();

  this->stateGamePlay->player->render();

  this->renderCreativeUi();

  if (debugMode) drawDegubInfo();

  Threading::switchThread();
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

  if (debugMode && clicked.Circle) {
    printMemoryInfoToLog();
    printBlockInfoToLog();
  }
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

  // Draw print log tip message:
  this->t_fontManager->printText(
      "Press circle to log debug info",
      FontOptions(Vec2(5.0f, 35.0f), Color(255), 0.8F));

  // Draw ticks
  std::string ticks =
      std::string("Ticks: ").append(std::to_string(g_ticksCounter));
  this->t_fontManager->printText(
      ticks, FontOptions(Vec2(5.0f, 50.0f), Color(255), 0.8F));

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
      playerStateInfo, FontOptions(Vec2(350.0f, 5.0f), Color(255), 0.8F));
}

void CreativePlayingState::printBlockInfoToLog() {
  return;
  // std::string blocksInfo = std::string("");
  // for (size_t i = (u8)Blocks::STONE_BLOCK; i < (u8)Blocks::TOTAL_OF_BLOCKS;
  //      i++) {
  //   BlockInfo info =
  //       *this->stateGamePlay->world->blockManager->getBlockTexOffsetByType(
  //           static_cast<Blocks>(i));

  //   blocksInfo.append(std::string("\nID: "))
  //       .append(std::to_string(i))
  //       .append(std::string(" | X: "))
  //       .append(std::to_string(info._texOffssetX))
  //       .append(std::string(" | Y: "))
  //       .append(std::to_string(info._texOffssetX));
  // }
  // TYRA_LOG(blocksInfo.c_str());
}

void CreativePlayingState::printMemoryInfoToLog() {
  Info info = this->stateGamePlay->context->t_engine->info;
  std::string freeRam = std::string("Free RAM: ")
                            .append(std::to_string(info.getAvailableRAM()))
                            .append(" MB");
  TYRA_LOG(freeRam.c_str());
}

void CreativePlayingState::playNewRandomSong() {
  TYRA_LOG("Song finished, playing a new random song.");
  this->t_creativeAudioListener->playRandomCreativeSound();
}
