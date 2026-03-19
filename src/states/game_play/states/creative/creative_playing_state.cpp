#include "states/game_play/states/creative/creative_playing_state.hpp"
#include "managers/save_manager.hpp"
#include "managers/settings_manager.hpp"
#include "managers/notification/notification_manager.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include "debug.hpp"
#include "utils.hpp"
#include "managers/background_task_service.hpp"

CreativePlayingState::CreativePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context),
      postFxManager(&t_context->context->t_engine->renderer),
      autoSaveTimer(g_settings.auto_save_interval),
      autoSaveWarningTimer(10.0f) {}

CreativePlayingState::~CreativePlayingState() {
  stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
#ifdef DEBUG_MODE
  g_debug_tick_scheduler = nullptr;
#endif
}

void CreativePlayingState::init() {
  creativeAudioListener.init(&stateGamePlay->context->t_engine->audio.song);
  audioListenerId = stateGamePlay->context->t_engine->audio.song.addListener(
      &creativeAudioListener);

  tickManager.onTick = [this]() { tick(); };
}

void CreativePlayingState::afterInit() {
  // World and player are fully initialized at this point
  stateGamePlay->world->setTickContext(stateGamePlay->player,
                                       stateGamePlay->context->t_camera);
  stateGamePlay->world->registerTickCallbacks(tickManager.scheduler);
  stateGamePlay->player->registerTickCallbacks(tickManager.scheduler);

  // RAII handle — auto-cancelled when CreativePlayingState is destroyed
  tickHandles.add(tickManager.scheduler.everyHandle(200, [this]() {
    if (!isSongPlaying()) playNewRandomSong();
  }));

#ifdef DEBUG_MODE
  g_debug_tick_scheduler = &tickManager.scheduler;
#endif
}

void CreativePlayingState::fixedUpdate(const float& fixedDeltaTime) {
  stateGamePlay->world->fixedUpdate(
      stateGamePlay->player, stateGamePlay->context->t_camera, fixedDeltaTime);

  if (isInventoryOpened()) playerMovementDirection = Vec4(0.0F);

  stateGamePlay->player->fixedUpdate(fixedDeltaTime, playerMovementDirection,
                                     stateGamePlay->context->t_camera);
}

void CreativePlayingState::update(const float& deltaTime) {
  elapsedTimeInSec += deltaTime;
  
  if (!isAutoSaveWarningActive && autoSaveTimer.update(deltaTime)) {
      isAutoSaveWarningActive = true;
      autoSaveWarningTimer.reset();
      autoSaveLastSecondsLeft = 5;
      autoSaveWarningNotification = NotificationManager::getInstance()->notify(Label_AutoSaveIn + std::to_string(5) + Label_Seconds, Label_PressSelectToCancel);
      if (autoSaveWarningNotification) {
          autoSaveWarningNotification->timeout = 10.0F;
      }
  }

  if (isAutoSaveWarningActive) {
      if (autoSaveWarningTimer.update(deltaTime)) {
          isAutoSaveWarningActive = false;
          if (autoSaveWarningNotification) {
              autoSaveWarningNotification->destroy = true;
              autoSaveWarningNotification = nullptr;
          }
          autoSave();
      } else {
          int secondsLeft = 5 - (int)autoSaveWarningTimer.current;
          if (secondsLeft != autoSaveLastSecondsLeft && secondsLeft >= 0) {
              autoSaveLastSecondsLeft = secondsLeft;
              if (autoSaveWarningNotification) {
                  autoSaveWarningNotification->title = Label_AutoSaveIn + std::to_string(secondsLeft) + Label_Seconds;
              }
          }
      }
  }

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

  // Pre-build debug strings while in update frame (zero allocation at render)
  if (g_debug_mode) {
    const Vec4 playerOffset = stateGamePlay->world->pLevel->worldPosToOffset(
        *stateGamePlay->player->getPosition());

    _dbg_seed = std::string("Seed: ").append(
        std::to_string(stateGamePlay->world->getSeed()));
    _dbg_fps = std::string("FPS: ").append(
        std::to_string(stateGamePlay->context->t_engine->info.getFps()));
    _dbg_ticks = std::string("Ticks: ").append(std::to_string(g_ticksCounter));
    _dbg_playerPos = std::string("Player Position ")
                         .append(" X: ")
                         .append(std::to_string((int)playerOffset.x))
                         .append("   Y: ")
                         .append(std::to_string((int)playerOffset.y))
                         .append("   Z: ")
                         .append(std::to_string((int)playerOffset.z));
    _dbg_visibleChunks = std::string("Visible chunks: ").append(
        std::to_string(
            static_cast<int>(stateGamePlay->world->getVisibleChunksCount())));
    _dbg_chunksToLoad = std::string("Chunks to load: ").append(
        std::to_string(
            static_cast<int>(stateGamePlay->world->getChunksToLoadCount())));
    _dbg_chunksToUnload = std::string("Chunks to unload: ").append(
        std::to_string(static_cast<int>(
            stateGamePlay->world->getChunksToUnloadCount())));
    _dbg_chunksToUpdateLight =
        std::string("Chunks to update light: ")
            .append(std::to_string(static_cast<int>(
                stateGamePlay->world->getChunksToUpdateLightCount())));
    _dbg_particles = std::string("Particles alive: ").append(
        std::to_string(
            stateGamePlay->world->particlesManager.getParticlesCounter()));
    _dbg_tickAvg = std::string("Tick avg speed: ").append(
        std::to_string(tickManager.getTickTimeAverage()));
    _dbg_loadQueueInfo = stateGamePlay->world->getLoadQueueDebugInfo();
    _dbg_version = std::string("Version: ").append(VERSION);
  }
}

void CreativePlayingState::processIdleWork() {
  stateGamePlay->world->processIdleWork();
}

void CreativePlayingState::tick() {
  stateGamePlay->world->tick();
  stateGamePlay->player->tick();
  stateGamePlay->ui->update();
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

  postFxManager.renderAll(Color(25, 25, 255));
  // postFxManager.renderFog(
  //     stateGamePlay->world->dayNightCycleManager.getSkyColor());

  // General 2D sftuff
  renderCreativeUi();

  if (g_debug_mode) drawDebugInfo();
  if (isInventoryOpened()) stateGamePlay->ui->renderInventoryMenu();
}

void CreativePlayingState::handleInput(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();
  const auto& pressed = stateGamePlay->context->t_engine->pad.getPressed();

  if (clicked.Select) {
    if (isAutoSaveWarningActive) {
      isAutoSaveWarningActive = false;
      autoSaveWarningTimer.reset();
      autoSaveTimer.reset();
      if (autoSaveWarningNotification) {
          autoSaveWarningNotification->destroy = true;
          autoSaveWarningNotification = nullptr;
      }
      NotificationManager::getInstance()->notify(Label_AutoSaveCancelled, Label_SkippedThisAutoSave);
    } else {
      g_debug_mode = !g_debug_mode;
      TYRA_LOG("Debug mode: ", g_debug_mode ? "ON" : "OFF");
    }
  }

  if (g_debug_mode) {
#ifdef DEBUG_MODE
    if (clicked.L1 && clicked.R1) {
      g_debug_menu.showDebugMenu = !g_debug_menu.showDebugMenu;
      TCLOG("Debug menu: %s", g_debug_menu.showDebugMenu ? "ON" : "OFF");
    }

    if (g_debug_menu.showDebugMenu) {
      handleDebugInput(&stateGamePlay->context->t_engine->pad);
      return;
    }
#endif  // end if DEBUG_MODE

    if (clicked.Circle) printMemoryInfoToLog();

    if (pressed.Square && clicked.DpadUp) {
      stateGamePlay->world->mobManager.spawnMobAtPosition(
          MobType::Pig, stateGamePlay->player->position);

      stateGamePlay->world->mobManager.spawnMobAtPosition(
          MobType::Cow, stateGamePlay->player->position);
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
    } else if (stateGamePlay->world->isBreakingBlock()) {
      stateGamePlay->world->stopBreakTargetBlock();
      stateGamePlay->player->setArmIdleAnimation();
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
          stateGamePlay->world->putBlock(blockid, stateGamePlay->player);
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
    } else if (stateGamePlay->player->isFlying()) {
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
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderUI == false) return;
#endif  // DEBUG_MODE

  if (stateGamePlay->player->isUnderWater())
    stateGamePlay->ui->renderUnderWaterOverlay();

  if (stateGamePlay->context->t_camera->getCamType() == CamType::FirstPerson)
    stateGamePlay->ui->renderCrosshair();

  stateGamePlay->ui->renderInventory();
}

void CreativePlayingState::drawDebugInfo() {
#ifdef DEBUG_MODE
  if (g_debug_menu.showDebugMenu) {
    renderDebugMenu();
    return;
  }
#endif  // end if DEBUG_MODE

  FontManager& fm = FontManager::getInstanceRef();
  float cursorY = 5.0f;

  // All strings are pre-built in update() — no heap allocation here
  fm.printText(_dbg_seed,              FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_fps,               FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 25.0f;
  fm.printText(_dbg_ticks,             FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_playerPos,         FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_visibleChunks,     FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_chunksToLoad,      FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_chunksToUnload,    FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_chunksToUpdateLight, FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_particles,         FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_tickAvg,           FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_loadQueueInfo,     FontOptions(Vec2(5.0f, cursorY), Color(255), 0.8F)); cursorY += 15.0f;
  fm.printText(_dbg_version,           FontOptions(Vec2(5.0f, 420.0f),  Color(255), 0.8F));

  renderOnScreenLogs();
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
  std::string savePath = stateGamePlay->world->getWorldOptions()->fullPath;

  SaveResult result = SaveManager::SaveGame(stateGamePlay, savePath.c_str());
  
  NotificationManager* instance = NotificationManager::getInstance();
  if (result) {
    TYRA_LOG("Saving at: ", savePath.c_str());
    instance->notify(Message_Saved_Successfully.c_str(),
                     Message_Progress_Has_Been_Saved.c_str());
  } else {
    TYRA_LOG("ERROR saving game: ", result.errorMessage.c_str());
    instance->notify("Save Failed", result.errorMessage.c_str());
  }
}

void CreativePlayingState::autoSave() {
  std::string savePath = stateGamePlay->world->getWorldOptions()->fullPath;

  // Status notification during save
  NotificationManager::getInstance()->notify(Label_AutoSave, Label_SavingDoNotTurnOff);

  std::string msgSaved = Message_Saved_Successfully;
  std::string msgProgress = Message_Progress_Has_Been_Saved;

  auto* bgService = BackgroundTaskService::getInstance();
  if (bgService) {
    auto state = stateGamePlay;
    bgService->submit(
        [state, savePath]() {
          SaveResult result = SaveManager::SaveGame(state, savePath.c_str());
          if (result) {
            TYRA_LOG("Auto-saving at: ", savePath.c_str());
          } else {
            TYRA_LOG("ERROR auto-saving: ", result.errorMessage.c_str());
          }
        },
        [msgSaved, msgProgress]() {
          NotificationManager* instance = NotificationManager::getInstance();
          instance->notify(msgSaved.c_str(), msgProgress.c_str());
        });
  } else {
    // Fallback sync save
    SaveResult result = SaveManager::SaveGame(stateGamePlay, savePath.c_str());
    TYRA_LOG("Auto-saving at: ", savePath.c_str());
    NotificationManager* instance = NotificationManager::getInstance();
    if (result) {
      instance->notify(msgSaved.c_str(), msgProgress.c_str());
    } else {
      instance->notify("Auto-Save Failed", result.errorMessage.c_str());
    }
  }
}
