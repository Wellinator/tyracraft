#include "states/game_play/states/maze/maze_playing_state.hpp"
#include "managers/settings_manager.hpp"
#include "managers/save_manager.hpp"
#include "debug.hpp"
#include "utils.hpp"

MazePlayingState::MazePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context) {}

MazePlayingState::~MazePlayingState() {
  stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
}

void MazePlayingState::init() {
  mazeAudioListener.init(&stateGamePlay->context->t_engine->audio.song);
  audioListenerId = stateGamePlay->context->t_engine->audio.song.addListener(
      &mazeAudioListener);

  tickManager.onTick = [this]() { tick(); };
}

void MazePlayingState::afterInit() {
  setDarkTheme();

  stateGamePlay->player->fillInventoryWithItem(ItemId::torch);
  stateGamePlay->player->updateHandledItem();
  stateGamePlay->player->unFly();
}

void MazePlayingState::update(const float& deltaTime) {
  if (deltaTime <= 0.0F) return;
  // elapsedTimeInSec += deltaTime;
  tickManager.update(deltaTime);

  handleInput(deltaTime);

  if (shouldLoadNextLevel) {
    return loadNextLevel();
  }

  stateGamePlay->world->update(stateGamePlay->player,
                               stateGamePlay->context->t_camera, deltaTime);

  stateGamePlay->player->update(deltaTime, playerMovementDirection,
                                stateGamePlay->context->t_camera,
                                stateGamePlay->world->terrain);

  stateGamePlay->context->t_camera->setPosition(
      *stateGamePlay->player->mesh->getPosition());
  stateGamePlay->context->t_camera->setLookDirectionByPad(
      &stateGamePlay->context->t_engine->pad, deltaTime);

  stateGamePlay->context->t_camera->update(deltaTime,
                                           stateGamePlay->player->isMoving);
}

void MazePlayingState::tick() {
  stateGamePlay->world->tick(stateGamePlay->player,
                             stateGamePlay->context->t_camera);
  stateGamePlay->player->tick(stateGamePlay->world->terrain);
  stateGamePlay->ui->update();

  if (!isSongPlaying() && isTicksCounterAt(200)) playNewRandomSong();
}

void MazePlayingState::render() {
  stateGamePlay->world->dayNightCycleManager.render();
  stateGamePlay->world->cloudsManager.render();
  stateGamePlay->world->mobManager.render();
  stateGamePlay->world->renderOpaque();
  stateGamePlay->world->renderTransparent();
  stateGamePlay->world->renderBlockDamageOverlay();
  stateGamePlay->player->render();
  stateGamePlay->world->particlesManager.render();
  renderMazeUi();

  if (g_debug_mode) drawDegubInfo();
}

void MazePlayingState::handleInput(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.Select) g_debug_mode = !g_debug_mode;
  if (g_debug_mode) {
    if (clicked.Circle) printMemoryInfoToLog();

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

  gamePlayInputHandler(deltaTime);
}

void MazePlayingState::gamePlayInputHandler(const float& deltaTime) {
  const auto& clicked = stateGamePlay->context->t_engine->pad.getClicked();
  const auto& pressed = stateGamePlay->context->t_engine->pad.getPressed();
  const auto& lJoyPad = stateGamePlay->context->t_engine->pad.getLeftJoyPad();

  // Player commands
  {
    // Check deadzone
    const auto _h = (lJoyPad.h - 128.0F) / 128.0F;
    const auto _v = (lJoyPad.v - 128.0F) / 128.0F;
    playerMovementDirection =
        Vec4(Utils::Abs(_h) > g_settings.l_stick_H ? _h : 0.0F, 0.0F,
             Utils::Abs(_v) > g_settings.l_stick_V ? _v : 0.0F);

    // Set running state
    stateGamePlay->player->setRunning((bool)pressed.Square);

    // if (clicked.L1)
    //   stateGamePlay->player->moveSelectorToTheLeft();
    // else if (clicked.R1)
    //   stateGamePlay->player->moveSelectorToTheRight();

    if (pressed.L2) {
      if (stateGamePlay->world->validTargetBlock()) {
        // TODO: check if target block is the final pupkin and load next level;
        if (hasReachedTargetBlock()) {
          // TODO: set happy layout and load next level
          shouldLoadNextLevel = true;
          return;
        }

        // prevent to break the scenario
        if (stateGamePlay->world->targetBlock->type != Blocks::TORCH) {
          return;
        }

        stateGamePlay->world->breakTargetBlockInCreativeMode(deltaTime);
        stateGamePlay->player->setArmBreakingAnimation();
      }
    } else if (stateGamePlay->world->isBreakingBLock()) {
      stateGamePlay->world->stopBreakTargetBlock();
      stateGamePlay->player->unsetArmBreakingAnimation();
    }

    if (clicked.R2 && stateGamePlay->world->validTargetBlock()) {
      const Blocks blockid = Blocks::TORCH;

      if (blockid != Blocks::AIR_BLOCK) {
        stateGamePlay->player->playPutBlockAnimation();
        stateGamePlay->world->putBlock(blockid, stateGamePlay->player,
                                       stateGamePlay->context->t_camera->yaw);

        if (hasReachedTargetBlock()) {
          // TODO: set happy layout and load next level
          shouldLoadNextLevel = true;
          return;
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
      // if (clicked.DpadUp)
      //   stateGamePlay->player->selectNextItem();
      // else if (clicked.DpadDown)
      //   stateGamePlay->player->selectPreviousItem();
    }

    // else if (stateGamePlay->player->isFlying) {
    // if (pressed.DpadUp) {
    //   stateGamePlay->player->flyUp(deltaTime);
    // } else if (pressed.DpadDown) {
    //   stateGamePlay->player->flyDown(deltaTime);
    // }
    // }

    // if (clicked.Cross) {
    //   if (elapsedTimeInSec < 0.45F) {
    //     stateGamePlay->player->toggleFlying();
    //   }
    //   elapsedTimeInSec = 0.0F;
    // }
  }
}

u8 MazePlayingState::hasReachedTargetBlock() {
  return stateGamePlay->world->validTargetBlock() &&
         stateGamePlay->world->targetBlock->type ==
             Blocks::JACK_O_LANTERN_BLOCK;
}

void MazePlayingState::setHappyTheme() {
  stateGamePlay->world->dayNightCycleManager.resetSkyColor();
  tickManager.setDayMid();
  stateGamePlay->world->chunckManager.reloadLightDataOfAllChunks();
}

void MazePlayingState::setDarkTheme() {
  Color darkColor = Color(AFTERNOON_MORNING_COLOR);
  NewGameOptions* worldOptions = stateGamePlay->world->getWorldOptions();
  u8 level = worldOptions->seed;

  if (level < 10) {
    darkColor = Color(AFTERNOON_MORNING_COLOR);
  } else if (level < 20) {
    darkColor = Color(35.0f, 58.0f, 97.0f);
  } else if (level < 30) {
    darkColor = Color(219.0f, 101.0f, 11.0f);
  } else if (level < 40) {
    darkColor = Color(97.0f, 14.0f, 8.0f);
  } else if (level < 50) {
    darkColor = Color(NIGHT_MID_COLOR);
  } else {
    darkColor = Color(AFTERNOON_MORNING_COLOR);
  }

  stateGamePlay->world->dayNightCycleManager.setSkyColor(darkColor, darkColor,
                                                         darkColor);
}

void MazePlayingState::navigate() {}

void MazePlayingState::renderMazeUi() {
  stateGamePlay->ui->renderCrosshair();

  // Do not render invetory, it just contains torch
  // stateGamePlay->ui->renderInventory();
}

void MazePlayingState::drawDegubInfo() {
  // Draw seed
  std::string seed = std::string("Seed: ").append(
      std::to_string(stateGamePlay->world->getSeed()));
  FontManager_printText(seed, FontOptions(Vec2(5.0f, 5.0f), Color(255), 0.8F));

  // Draw FPS:
  std::string fps = std::string("FPS: ").append(
      std::to_string(stateGamePlay->context->t_engine->info.getFps()));
  FontManager_printText(fps, FontOptions(Vec2(5.0f, 20.0f), Color(255), 0.8F));

  // Draw ticks
  std::string ticks =
      std::string("Ticks: ").append(std::to_string(g_ticksCounter));
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

  // Draw particles counter
  std::string particle_counter =
      std::string("Particles alive: ")
          .append(std::to_string(
              stateGamePlay->world->particlesManager.getParticlesCounter()));
  FontManager_printText(particle_counter,
                        FontOptions(Vec2(5.0f, 130.0f), Color(255), 0.8F));
  // Draw tick avg
  std::string tick_avg =
      std::string("Tick avg speed")
          .append(std::to_string(tickManager.getTickTimeAverage()));
  FontManager_printText(tick_avg,
                        FontOptions(Vec2(5.0f, 145.0f), Color(255), 0.8F));

  // Draw version
  std::string version = std::string("Version: ").append(VERSION);
  FontManager_printText(version,
                        FontOptions(Vec2(5.0f, 420.0f), Color(255), 0.8F));
}

void MazePlayingState::printMemoryInfoToLog() {
  std::string freeRam =
      std::string("Free RAM: ")
          .append(std::to_string(
              stateGamePlay->context->t_engine->info.getAvailableRAM()))
          .append(" MB");
  TYRA_LOG(freeRam.c_str());
}

void MazePlayingState::playNewRandomSong() {
  TYRA_LOG("Song finished, playing a new random song.");
  mazeAudioListener.playRandomMazeSound();
}

void MazePlayingState::handleAction(MenuAction action) {
  switch (action) {
    case MenuAction::Save:
      saveProgress();
      break;

    default:
      break;
  }
}

void MazePlayingState::saveProgress() {
  std::string saveFileName = FileUtils::fromCwd(
      "saves/" + stateGamePlay->world->getWorldOptions()->name + "." +
      MINIGAME_FILE_EXTENSION);
  SaveManager::SaveGame(stateGamePlay, saveFileName.c_str());
  TYRA_LOG("Saving mazecraft at: ", saveFileName.c_str());
}

void MazePlayingState::loadNextLevel() {
  setHappyTheme();
  NewGameOptions model = *stateGamePlay->world->getWorldOptions();
  model.seed += 1;
  TYRA_LOG("Generating level: ", model.seed);
  stateGamePlay->loadNextMiniGameLevel(model);
}
