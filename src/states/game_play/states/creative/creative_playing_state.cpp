#include "states/game_play/states/creative/creative_playing_state.hpp"
#include "managers/settings_manager.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {}

CreativePlayingState::~CreativePlayingState() {
  stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
}

void CreativePlayingState::init() {
  creativeAudioListener.init(&stateGamePlay->context->t_engine->audio.song);
  audioListenerId = stateGamePlay->context->t_engine->audio.song.addListener(
      &creativeAudioListener);
}

void CreativePlayingState::update(const float& deltaTime) {
  if (deltaTime <= 0.0F) return;
  elapsedTimeInSec += deltaTime;
  tickManager.update(deltaTime);

  handleInput(deltaTime);

  stateGamePlay->world->update(stateGamePlay->player,
                               stateGamePlay->context->t_camera, deltaTime);

  if (isInventoryOpened()) playerMovementDirection = Vec4(0.0F);

  stateGamePlay->player->update(
      deltaTime, playerMovementDirection, stateGamePlay->context->t_camera,
      stateGamePlay->world->chunckManager.getNearByChunks(), &terrainHeight,
      stateGamePlay->world->terrain);

  stateGamePlay->ui->update();

  stateGamePlay->context->t_camera->setPositionByMesh(
      stateGamePlay->player->mesh.get());
  stateGamePlay->context->t_camera->setLookDirectionByPad(
      &stateGamePlay->context->t_engine->pad);
  stateGamePlay->context->t_camera->update();

  if (!isSongPlaying()) playNewRandomSong();
}

void CreativePlayingState::render() {
  stateGamePlay->world->render();

  stateGamePlay->player->render();

  renderCreativeUi();

  if (isInventoryOpened()) stateGamePlay->ui->renderInventoryMenu();

  if (debugMode) drawDegubInfo();
}

void CreativePlayingState::handleInput(const float& deltaTime) {
  // FIX: camera moving while in inventory
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.Select) debugMode = !debugMode;
  if (debugMode && clicked.Circle) printMemoryInfoToLog();

  if (isInventoryOpened()) {
    inventoryInputHandler(deltaTime);
  } else {
    gamePlayInputHandler(deltaTime);
  }
}

void CreativePlayingState::gamePlayInputHandler(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();
  const auto& pressed = stateGamePlay->context->t_engine->pad.getPressed();
  const auto& lJoyPad = stateGamePlay->context->t_engine->pad.getLeftJoyPad();

  if (clicked.Triangle && !isInventoryOpened()) openInventory();

  // Player commands
  {
    // Check deadzone
    const auto _h = (lJoyPad.h - 128.0F) / 128.0F;
    const auto _v = (lJoyPad.v - 128.0F) / 128.0F;
    playerMovementDirection =
        Vec4(std::abs(_h) > g_settings.l_stick_H ? _h : 0.0F, 0.0F,
             std::abs(_v) > g_settings.l_stick_V ? _v : 0.0F);

    terrainHeight = stateGamePlay->player->getTerrainHeightAtPosition(
        stateGamePlay->world->chunckManager.getVisibleChunks());

    // Set running state
    stateGamePlay->player->setRunning((bool)pressed.Square);

    if (clicked.L1)
      stateGamePlay->player->moveSelectorToTheLeft();
    else if (clicked.R1)
      stateGamePlay->player->moveSelectorToTheRight();

    if (pressed.L2) {
      if (stateGamePlay->world->validTargetBlock()) {
        stateGamePlay->world->breakTargetBlock(deltaTime);
        stateGamePlay->player->setArmBreakingAnimation();
      }
    } else if (stateGamePlay->world->isBreakingBLock()) {
      stateGamePlay->world->stopBreakTargetBlock();
      stateGamePlay->player->unsetArmBreakingAnimation();
    }

    if (clicked.R2) {
      ItemId activeItemType =
          stateGamePlay->player->getSelectedInventoryItemType();
      if (activeItemType != ItemId::empty &&
          stateGamePlay->world->validTargetBlock()) {
        const Blocks blockid =
            stateGamePlay->itemRepository->getItemById(activeItemType)->blockId;
        if (blockid != Blocks::AIR_BLOCK)
          stateGamePlay->world->putBlock(blockid, stateGamePlay->player);
      }
    }

    if (stateGamePlay->player->isOnWater() ||
        stateGamePlay->player->isUnderWater()) {
      if (pressed.Cross) stateGamePlay->player->swim();
    }

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

    if (clicked.Cross) {
      if (elapsedTimeInSec < 0.45F) {
        stateGamePlay->player->toggleFlying();
      }
      elapsedTimeInSec = 0.0F;
    }

    if (clicked.R3) {
      stateGamePlay->player->toogleCameraMode();

      if (stateGamePlay->player->isFirstPerson) {
        stateGamePlay->context->t_camera->setFirstPerson();
      } else {
        stateGamePlay->context->t_camera->setThirdPerson();
      }
    }
  }
}

void CreativePlayingState::inventoryInputHandler(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();

  Inventory& creativeInvetory = *stateGamePlay->ui->getInvetory();

  if (clicked.DpadUp)
    creativeInvetory.moveSelectorUp();
  else if (clicked.DpadDown)
    creativeInvetory.moveSelectorDown();
  else if (clicked.DpadLeft)
    creativeInvetory.moveSelectorLeft();
  else if (clicked.DpadRight)
    creativeInvetory.moveSelectorRight();

  if (clicked.L1) stateGamePlay->player->moveSelectorToTheLeft();
  if (clicked.R1) stateGamePlay->player->moveSelectorToTheRight();

  if (clicked.Cross) {
    stateGamePlay->player->setItemToInventory(
        creativeInvetory.getSelectedItem());
  }

  if (clicked.Circle) closeInventory();
}

void CreativePlayingState::navigate() {}

void CreativePlayingState::renderCreativeUi() {
  if (stateGamePlay->player->isUnderWater())
    stateGamePlay->ui->renderUnderWaterOverlay();

  stateGamePlay->ui->renderCrosshair();
  stateGamePlay->ui->renderInventory();
}

void CreativePlayingState::drawDegubInfo() {
  // Draw seed
  std::string seed = std::string("Seed: ").append(
      std::to_string(stateGamePlay->world->getSeed()));
  FontManager_printText(seed, FontOptions(Vec2(5.0f, 5.0f), Color(255), 0.8F));

  // Draw FPS:
  std::string fps = std::string("FPS: ").append(
      std::to_string(stateGamePlay->context->t_engine->info.getFps()));
  FontManager_printText(fps, FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.8F));

  // Draw ticks
  std::string ticks = std::string("Ticks: ").append(
      std::to_string(static_cast<int>(g_ticksCounter)));
  FontManager_printText(ticks,
                        FontOptions(Vec2(5.0f, 45.0f), Color(255), 0.8F));
  // Draw Player Position
  const Vec4 pos = *stateGamePlay->player->getPosition();
  std::string playerPosition =
      std::string("Player Position ")
          .append(" X: ")
          .append(std::to_string(static_cast<int>(pos.x / DUBLE_BLOCK_SIZE)))
          .append("   Y: ")
          .append(std::to_string(static_cast<int>(pos.y / DUBLE_BLOCK_SIZE)))
          .append("   Z: ")
          .append(std::to_string(static_cast<int>(pos.z / DUBLE_BLOCK_SIZE)));
  FontManager_printText(playerPosition,
                        FontOptions(Vec2(5.0f, 65.0f), Color(255), 0.8F));

  // Draw chunks info
  std::string chunksToLoad =
      std::string("Chunks to load: ")
          .append(std::to_string(
              static_cast<int>(stateGamePlay->world->getChunksToLoadCount())));
  FontManager_printText(chunksToLoad,
                        FontOptions(Vec2(5.0f, 85.0f), Color(255), 0.8F));

  std::string chunksToUnload =
      std::string("Chunks to unload: ")
          .append(std::to_string(static_cast<int>(
              stateGamePlay->world->getChunksToUnloadCount())));
  FontManager_printText(chunksToUnload,
                        FontOptions(Vec2(5.0f, 100.0f), Color(255), 0.8F));

  std::string chunksToUpdateLight =
      std::string("Chunks to update light: ")
          .append(std::to_string(static_cast<int>(
              stateGamePlay->world->getChuncksToUpdateLightCount())));
  FontManager_printText(chunksToUpdateLight,
                        FontOptions(Vec2(5.0f, 115.0f), Color(255), 0.8F));

  // Draw version
  std::string version = std::string("Version: ").append(VERSION);
  FontManager_printText(version,
                        FontOptions(Vec2(5.0f, 420.0f), Color(255), 0.8F));
}

void CreativePlayingState::printMemoryInfoToLog() {
  std::string freeRam =
      std::string("Free RAM: ")
          .append(std::to_string(
              stateGamePlay->context->t_engine->info.getAvailableRAM()))
          .append(" MB");
  TYRA_LOG(freeRam.c_str());
}

void CreativePlayingState::playNewRandomSong() {
  TYRA_LOG("Song finished, playing a new random song.");
  creativeAudioListener.playRandomCreativeSound();
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
