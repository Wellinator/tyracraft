#include "states/game_play/states/creative/creative_playing_state.hpp"
#include "managers/save_manager.hpp"
#include "managers/settings_manager.hpp"
#include "managers/notification/notification_manager.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include "debug.hpp"
#include "utils.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context),
      postFxManager(&t_context->context->t_engine->renderer) {}

CreativePlayingState::~CreativePlayingState() {
  stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
}

void CreativePlayingState::init() {
  creativeAudioListener.init(&stateGamePlay->context->t_engine->audio.song);
  audioListenerId = stateGamePlay->context->t_engine->audio.song.addListener(
      &creativeAudioListener);

  tickManager.onTick = [this]() { tick(); };
}

void CreativePlayingState::fixedUpdate(const float& fixedDeltaTime) {
  stateGamePlay->world->fixedUpdate(
      stateGamePlay->player, stateGamePlay->context->t_camera, fixedDeltaTime);

  if (isInventoryOpened()) playerMovementDirection = Vec4(0.0F);

  stateGamePlay->player->fixedUpdate(fixedDeltaTime, playerMovementDirection,
                                     stateGamePlay->context->t_camera);
}

void CreativePlayingState::update(const float& deltaTime) {
  if (deltaTime <= 0.0F) return;
  elapsedTimeInSec += deltaTime;
  tickManager.update(deltaTime);

  handleInput(deltaTime);

  stateGamePlay->world->update(stateGamePlay->player,
                               stateGamePlay->context->t_camera, deltaTime);
  stateGamePlay->player->update(deltaTime, stateGamePlay->context->t_camera);
  stateGamePlay->context->t_camera->setPosition(
      stateGamePlay->player->position);
  stateGamePlay->context->t_camera->setLookDirectionByPad(
      &stateGamePlay->context->t_engine->pad, deltaTime);

  stateGamePlay->context->t_camera->update(deltaTime,
                                           stateGamePlay->player->isMoving);
}

void CreativePlayingState::tick() {
  stateGamePlay->world->tick(stateGamePlay->player,
                             stateGamePlay->context->t_camera);
  stateGamePlay->player->tick();
  stateGamePlay->ui->update();

  if (!isSongPlaying() && isTicksCounterAt(200)) playNewRandomSong();
}

void CreativePlayingState::render() {
  // General 3D sftuff
  stateGamePlay->world->dayNightCycleManager.render();
  stateGamePlay->world->cloudsManager.render();
  stateGamePlay->world->renderOpaque();
  stateGamePlay->world->mobManager.render();
  stateGamePlay->world->particlesManager.render();
  stateGamePlay->player->render();

  // General 3D sftuff with transparency
  stateGamePlay->world->renderTransparent();
  stateGamePlay->world->renderBlockDamageOverlay();

  // PostFX
  // postFxManager.render(
  //     stateGamePlay->world->dayNightCycleManager.getSkyColor());

  // General 2D sftuff
  renderCreativeUi();

  if (g_debug_mode) drawDegubInfo();
  if (isInventoryOpened()) stateGamePlay->ui->renderInventoryMenu();
}

void CreativePlayingState::handleInput(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();
  const auto& pressed = stateGamePlay->context->t_engine->pad.getPressed();

  if (clicked.Select) g_debug_mode = !g_debug_mode;
  if (g_debug_mode) {
    if (clicked.Circle) printMemoryInfoToLog();

    if (pressed.Square && clicked.DpadUp) {
      stateGamePlay->world->mobManager.spawnMobAtPosition(
          MobType::Pig, stateGamePlay->player->position);
    }

    // List loaded textures and VRAM
    if (clicked.Triangle) {
      TYRA_LOG("-----------FREE VRAM-----------");
      TYRA_LOG(stateGamePlay->context->t_engine->renderer.core.gs.vram
                   .getFreeSpaceInMB(),
               "MB");

      auto& texRepo =
          stateGamePlay->context->t_engine->renderer.getTextureRepository();
      TYRA_LOG("---------TEXTURES---------");
      TYRA_LOG("Total of loaded textures: ", (int)texRepo.getTexturesCount());

      std::vector<Texture*>* textures = texRepo.getAll();
      for (size_t i = 0; i < textures->size(); i++) {
        auto tex = textures->at(i);
        TYRA_LOG(i, ": ", tex->name.c_str(), ", ", tex->getSizeInMB(), "MB.");
      }
      TYRA_LOG("---------------------------");
    }
  }

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
        Vec4(Utils::Abs(_h) > g_settings.l_stick_H ? _h : 0.0F, 0.0F,
             Utils::Abs(_v) > g_settings.l_stick_V ? _v : 0.0F)
            .getNormalized();

    // Set running state
    stateGamePlay->player->setRunning((bool)pressed.Square);

    if (clicked.L1)
      stateGamePlay->player->moveSelectorToTheLeft();
    else if (clicked.R1)
      stateGamePlay->player->moveSelectorToTheRight();

    if (pressed.L2) {
      if (stateGamePlay->world->validTargetBlock()) {
        stateGamePlay->world->breakTargetBlockInCreativeMode(deltaTime);
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
        if (blockid != Blocks::AIR_BLOCK) {
          stateGamePlay->player->playPutBlockAnimation();
          stateGamePlay->world->putBlock(blockid, stateGamePlay->player,
                                         stateGamePlay->context->t_camera->yaw);
        }
      }
    } else {
      stateGamePlay->player->stopPutBlockAnimation();
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
        stateGamePlay->player->flyUp(deltaTime);
      } else if (pressed.DpadDown) {
        stateGamePlay->player->flyDown(deltaTime);
      }
    }

    if (clicked.Cross) {
      if (elapsedTimeInSec < 0.45F) {
        stateGamePlay->player->toggleFlying();
      }
      elapsedTimeInSec = 0.0F;
    }

    if (clicked.R3) {
      Camera* t_cam = stateGamePlay->context->t_camera;

      if (t_cam->getCamType() == CamType::FirstPerson) {
        t_cam->setThirdPerson();
      } else if (t_cam->getCamType() == CamType::ThirdPerson) {
        t_cam->setFirstPerson();
      }

      // TODO: Implements inverted third person cam
      // else if (t_cam->getCamType() == CamType::ThirdPersonInverted) {
      //   t_cam->setFirstPerson();
      // }

      if (t_cam->getCamType() == CamType::FirstPerson) {
        stateGamePlay->player->setRenderArmPip();
      } else {
        stateGamePlay->player->setRenderBodyPip();
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

  if (stateGamePlay->context->t_camera->getCamType() == CamType::FirstPerson)
    stateGamePlay->ui->renderCrosshair();

  stateGamePlay->ui->renderInventory();
}

void CreativePlayingState::drawDegubInfo() {
  FontManager& fm = FontManager::getInstanceRef();

  // Draw seed
  std::string seed = std::string("Seed: ").append(
      std::to_string(stateGamePlay->world->getSeed()));
  fm.printText(seed, FontOptions(Vec2(5.0f, 5.0f), Color(255), 0.8F));

  // Draw FPS:
  std::string fps = std::string("FPS: ").append(
      std::to_string(stateGamePlay->context->t_engine->info.getFps()));
  fm.printText(fps, FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.8F));

  // Draw ticks
  std::string ticks =
      std::string("Ticks: ").append(std::to_string(g_ticksCounter));
  fm.printText(ticks, FontOptions(Vec2(5.0f, 45.0f), Color(255), 0.8F));
  // Draw Player Position
  const Vec4 playerOffset = stateGamePlay->world->pLevel->worldPosToOffset(
      *stateGamePlay->player->getPosition());
  std::string playerPosition = std::string("Player Position ")
                                   .append(" X: ")
                                   .append(std::to_string((int)playerOffset.x))
                                   .append("   Y: ")
                                   .append(std::to_string((int)playerOffset.y))
                                   .append("   Z: ")
                                   .append(std::to_string((int)playerOffset.z));
  fm.printText(playerPosition,
               FontOptions(Vec2(5.0f, 65.0f), Color(255), 0.8F));

  // Draw chunks info
  std::string chunksToLoad =
      std::string("Chunks to load: ")
          .append(std::to_string(
              static_cast<int>(stateGamePlay->world->getChunksToLoadCount())));
  fm.printText(chunksToLoad, FontOptions(Vec2(5.0f, 85.0f), Color(255), 0.8F));

  std::string chunksToUnload =
      std::string("Chunks to unload: ")
          .append(std::to_string(static_cast<int>(
              stateGamePlay->world->getChunksToUnloadCount())));
  fm.printText(chunksToUnload,
               FontOptions(Vec2(5.0f, 100.0f), Color(255), 0.8F));

  std::string chunksToUpdateLight =
      std::string("Chunks to update light: ")
          .append(std::to_string(static_cast<int>(
              stateGamePlay->world->getChunksToUpdateLightCount())));
  fm.printText(chunksToUpdateLight,
               FontOptions(Vec2(5.0f, 115.0f), Color(255), 0.8F));

  // Draw particles counter
  std::string particle_counter =
      std::string("Particles alive: ")
          .append(std::to_string(
              stateGamePlay->world->particlesManager.getParticlesCounter()));
  fm.printText(particle_counter,
               FontOptions(Vec2(5.0f, 130.0f), Color(255), 0.8F));
  // Draw tick avg
  std::string tick_avg =
      std::string("Tick avg speed")
          .append(std::to_string(tickManager.getTickTimeAverage()));
  fm.printText(tick_avg, FontOptions(Vec2(5.0f, 145.0f), Color(255), 0.8F));

  // Draw version
  std::string version = std::string("Version: ").append(VERSION);
  fm.printText(version, FontOptions(Vec2(5.0f, 420.0f), Color(255), 0.8F));
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

void CreativePlayingState::handleAction(MenuAction action) {
  switch (action) {
    case MenuAction::Save:
      saveProgress();
      break;

    default:
      break;
  }
}

void CreativePlayingState::saveProgress() {
  std::string saveFileName = FileUtils::fromCwd(
      "saves/" + stateGamePlay->world->getWorldOptions()->name + "." +
      SAVE_FILE_EXTENSION);
  SaveManager::SaveGame(stateGamePlay, saveFileName.c_str());
  TYRA_LOG("Saving at: ", saveFileName.c_str());

  NotificationManager* instance = NotificationManager::getInstance();
  instance->notify(Message_Saved_Successfully.c_str(),
                   Message_Progress_Has_Been_Saved.c_str());
}
