#include "states/game_play/states/creative/creative_playing_state.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {
  this->t_fontManager =
      new FontManager(&t_context->context->t_engine->renderer);
}

CreativePlayingState::~CreativePlayingState() {
  stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
  delete this->t_creativeAudioListener;
  delete this->t_fontManager;
}

void CreativePlayingState::init() {
  this->t_creativeAudioListener =
      new CreativeAudioListener(&stateGamePlay->context->t_engine->audio.song);
  this->audioListenerId =
      stateGamePlay->context->t_engine->audio.song.addListener(
          this->t_creativeAudioListener);
  this->t_creativeAudioListener->playRandomCreativeSound();
}

void CreativePlayingState::update(const float& deltaTime) {
  elapsedTimeInSec += deltaTime;
  tickManager.update(std::min(deltaTime, MAX_FRAME_MS));

  this->handleInput(deltaTime);

  stateGamePlay->world->update(stateGamePlay->player,
                               stateGamePlay->context->t_camera->lookPos,
                               stateGamePlay->context->t_camera->position);

  stateGamePlay->player->update(
      deltaTime, playerMovementDirection,
      stateGamePlay->context->t_camera->unitCirclePosition.getNormalized(),
      stateGamePlay->world->chunckManager->getVisibleChunks(), &terrainHeight);

  stateGamePlay->ui->update();

  stateGamePlay->context->t_camera->update(
      stateGamePlay->context->t_engine->pad, *stateGamePlay->player->mesh);

  if (!isSongPlaying()) playNewRandomSong();

  Threading::switchThread();
}

void CreativePlayingState::render() {
  stateGamePlay->world->render();

  stateGamePlay->player->render();

  this->renderCreativeUi();

  if (isInventoryOpened()) stateGamePlay->ui->renderInventoryMenu();

  if (debugMode) drawDegubInfo();

  Threading::switchThread();
}

void CreativePlayingState::handleInput(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();
  const auto& pressed = stateGamePlay->context->t_engine->pad.getPressed();
  const auto& lJoyPad = stateGamePlay->context->t_engine->pad.getLeftJoyPad();

  // Player commands
  {
    playerMovementDirection = Vec4((lJoyPad.h - 128.0F) / 128.0F, 0.0F,
                                   (lJoyPad.v - 128.0F) / 128.0F);
    terrainHeight = stateGamePlay->player->getTerrainHeightAtPosition(
        stateGamePlay->world->chunckManager->getVisibleChunks());

    if (clicked.L1) stateGamePlay->player->moveSelectorToTheLeft();
    if (clicked.R1) stateGamePlay->player->moveSelectorToTheRight();

    if (pressed.L2) {
      if (stateGamePlay->world->terrainManager->validTargetBlock()) {
        stateGamePlay->world->terrainManager->breakTargetBlock(deltaTime);
        stateGamePlay->player->setArmBreakingAnimation();
      }
    } else if (stateGamePlay->world->terrainManager->isBreakingBLock()) {
      stateGamePlay->world->terrainManager->stopBreakTargetBlock();
    }

    if (clicked.R2) {
      ItemId activeItemType =
          stateGamePlay->player->getSelectedInventoryItemType();
      if (activeItemType != ItemId::empty) {
        const Blocks blockid =
            stateGamePlay->itemRepository->getItemById(activeItemType)->blockId;
        if (blockid != Blocks::AIR_BLOCK)
          stateGamePlay->world->terrainManager->putBlock(blockid,
                                                         stateGamePlay->player);
      }
    }

    stateGamePlay->player->unsetArmBreakingAnimation();

    if (stateGamePlay->player->isOnGround) {
      if (pressed.Cross) stateGamePlay->player->jump();
      if (clicked.DpadUp)
        stateGamePlay->player->selectNextItem();
      else if (clicked.DpadDown)
        stateGamePlay->player->selectPreviousItem();
    } else if (stateGamePlay->player->isFlying) {
      if (pressed.DpadUp) {
        stateGamePlay->player->flyUp(deltaTime, terrainHeight);
      } else if (pressed.DpadDown) {
        stateGamePlay->player->flyDown(deltaTime, terrainHeight);
      }
    }
  }

  if (clicked.Triangle) {
    isInventoryOpened() ? closeInventory() : openInventory();
  }
  if (isInventoryOpened()) return;

  if (clicked.Cross) {
    if (elapsedTimeInSec < 0.5F) {
      stateGamePlay->player->toggleFlying();
    }
    elapsedTimeInSec = 0.0F;
  }

  if (clicked.Select) debugMode = !debugMode;

  if (debugMode && clicked.Circle) {
    printMemoryInfoToLog();
  }
}

void CreativePlayingState::navigate() {}

void CreativePlayingState::renderCreativeUi() {
  stateGamePlay->ui->renderCrosshair();
  stateGamePlay->ui->renderInventory();
}

void CreativePlayingState::drawDegubInfo() {
  Info* info = &stateGamePlay->context->t_engine->info;

  // Draw seed
  std::string seed = std::string("Seed: ").append(
      std::to_string(stateGamePlay->world->getSeed()));
  this->t_fontManager->printText(
      seed, FontOptions(Vec2(5.0f, 5.0f), Color(255), 0.8F));

  // Draw FPS:
  std::string fps = std::string("FPS: ").append(std::to_string(info->getFps()));
  this->t_fontManager->printText(
      fps, FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.8F));

  // Draw ticks
  std::string ticks =
      std::string("Ticks: ").append(std::to_string(g_ticksCounter));
  this->t_fontManager->printText(
      ticks, FontOptions(Vec2(5.0f, 50.0f), Color(255), 0.8F));
}

void CreativePlayingState::printMemoryInfoToLog() {
  Info info = stateGamePlay->context->t_engine->info;
  std::string freeRam = std::string("Free RAM: ")
                            .append(std::to_string(info.getAvailableRAM()))
                            .append(" MB");
  TYRA_LOG(freeRam.c_str());
}

void CreativePlayingState::playNewRandomSong() {
  TYRA_LOG("Song finished, playing a new random song.");
  this->t_creativeAudioListener->playRandomCreativeSound();
}

void CreativePlayingState::closeInventory() {
  stateGamePlay->ui->unloadInventory();
}

void CreativePlayingState::openInventory() {
  stateGamePlay->ui->loadInventory();
}

const u8 CreativePlayingState::isInventoryOpened() {
  return stateGamePlay->ui->isInventoryOpened();
}
