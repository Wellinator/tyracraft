#include "states/game_play/states/minigame/mazecraft/maze_playing_state.hpp"
#include "models/new_game_model.hpp"
#include "managers/settings_manager.hpp"
#include "managers/save_manager.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/font/font_options.hpp"
#include "debug.hpp"
#include "utils.hpp"

using Tyra::Color;
using Tyra::PadButtons;

MazePlayingState::MazePlayingState(StateGamePlay* t_context)
    : PlayingStateBase(t_context),
      postFxManager(&t_context->context->t_engine->renderer) {}

MazePlayingState::~MazePlayingState() {
  stateGamePlay->context->t_engine->audio.song.removeListener(
      this->audioListenerId);
#ifdef DEBUG_MODE
  g_debug_tick_scheduler = nullptr;
#endif

  Renderer* t_renderer = &stateGamePlay->context->t_engine->renderer;
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  textureRepo->freeBySprite(overlay);
  textureRepo->freeBySprite(btnL2);
}

void MazePlayingState::init() {
  mazeAudioListener.init(&stateGamePlay->context->t_engine->audio.song);
  audioListenerId = stateGamePlay->context->t_engine->audio.song.addListener(
      &mazeAudioListener);

  tickManager.onTick = [this]() { tick(); };

  Renderer* t_renderer = &stateGamePlay->context->t_engine->renderer;
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  const float halfWidth = t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = t_renderer->core.getSettings().getHeight() / 2;

  // Buttons
  btnL2.mode = Tyra::MODE_STRETCH;
  btnL2.size.set(32, 32);
  btnL2.position.set(15, t_renderer->core.getSettings().getHeight() - 35);

  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_L2.png"))
      ->addLink(btnL2.id);

  // Overlay
  overlay.mode = Tyra::MODE_STRETCH;
  overlay.size.set(halfWidth * 2, halfHeight * 2);
  overlay.position.set(0, 0);

  textureRepo->add(FileUtils::fromCwd("textures/gui/game_menu_overlay.png"))
      ->addLink(overlay.id);
}

void MazePlayingState::afterInit() {
  // World and player are fully initialized at this point
  stateGamePlay->world->setTickContext(stateGamePlay->player,
                                       stateGamePlay->context->t_camera);
  stateGamePlay->world->registerTickCallbacks(tickManager.scheduler);
  stateGamePlay->player->registerTickCallbacks(tickManager.scheduler);

  // RAII handle — auto-cancelled when MazePlayingState is destroyed
  tickHandles.add(tickManager.scheduler.everyHandle(200, [this]() {
    if (!mazeAudioListener.t_song->isPlaying())
      mazeAudioListener.playRandomMazeSound();
  }));

  setDarkTheme();
  stateGamePlay->player->fillInventoryWithItem(ItemId::torch);
  stateGamePlay->player->updateHandledItem();
  stateGamePlay->player->unFly();
  stateGamePlay->world->setDrawDistanceMode(DrawDistanceMode::Low);

#ifdef DEBUG_MODE
  g_debug_tick_scheduler = &tickManager.scheduler;
#endif
}

void MazePlayingState::fixedUpdate(const float& fixedDeltaTime) {
  stateGamePlay->world->fixedUpdate(
      stateGamePlay->player, stateGamePlay->context->t_camera, fixedDeltaTime);

  stateGamePlay->player->fixedUpdate(fixedDeltaTime, playerMovementDirection,
                                     stateGamePlay->context->t_camera);
}

void MazePlayingState::update(const float& deltaTime) {
  if (deltaTime <= 0.0F) return;
  elapsedTimeInSec += deltaTime;
  tickManager.update(deltaTime);

  if (shouldRenderLevelDoneDialog) {
    _nextLevelCounter -= deltaTime;
    shouldLoadNextLevel = _nextLevelCounter <= 0;

    if (shouldLoadNextLevel) {
      return loadNextLevel();
    }

    return;
  }

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

void MazePlayingState::tick() {
  stateGamePlay->world->tick();
  stateGamePlay->player->tick();
  stateGamePlay->ui->update();
}

void MazePlayingState::render() {
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

  // PostFX - fog + bloom (se habilitado) + restore
  postFxManager.renderAll(
      stateGamePlay->world->dayNightCycleManager.getSkyColor());

  // General 2D sftuff
  renderMazeUi();

  if (g_debug_mode) drawDebugInfo();
  if (shouldRenderLevelDoneDialog) renderCountDown();
}

void MazePlayingState::handleInput(const float& deltaTime) {
  const PadButtons& clicked =
      stateGamePlay->context->t_engine->pad.getClicked();

  if (clicked.Select) g_debug_mode = !g_debug_mode;
  if (g_debug_mode) {
#ifdef DEBUG_MODE
    if (clicked.L1 && clicked.R1) {
      g_debug_menu.showDebugMenu = !g_debug_menu.showDebugMenu;
      TYRA_LOG("Debug menu: ", g_debug_menu.showDebugMenu ? "ON" : "OFF");
    }

    if (g_debug_menu.showDebugMenu) {
      handleDebugInput(&stateGamePlay->context->t_engine->pad);
      return;
    }
#endif  // end if DEBUG_MODE

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
             Utils::Abs(_v) > g_settings.l_stick_V ? _v : 0.0F)
            .getNormalized();

    // Set running state
    stateGamePlay->player->setRunning((bool)pressed.Square);

    if (pressed.L2) {
      if (stateGamePlay->world->validTargetBlock()) {
        // TODO: check if target block is the final pupkin and load next level;
        if (hasReachedTargetBlock()) {
          shouldRenderLevelDoneDialog = true;
          setHappyTheme();
          mazeAudioListener.playLevelDoneSound();
          return;
        }

        // prevent to break the scenario
        if (stateGamePlay->world->targetBlock->getType() != Blocks::TORCH) {
          return;
        }

        stateGamePlay->world->breakTargetBlockInCreativeMode(deltaTime);
        stateGamePlay->player->setArmBreakingAnimation();
      }
    } else if (stateGamePlay->world->isBreakingBlock()) {
      stateGamePlay->world->stopBreakTargetBlock();
      stateGamePlay->player->setArmIdleAnimation();
    }

    if (clicked.R2 && stateGamePlay->world->validTargetBlock()) {
      stateGamePlay->player->playPutBlockAnimation();
      stateGamePlay->world->putBlock(Blocks::TORCH, stateGamePlay->player);
    } else {
      stateGamePlay->player->stopPutBlockAnimation();
    }

    if (stateGamePlay->player->isOnWater() ||
        stateGamePlay->player->isUnderWater()) {
      if (pressed.Cross) stateGamePlay->player->swim();
    }

    if (stateGamePlay->player->isOnGround) {
      if (pressed.Cross) stateGamePlay->player->jump();
    }

    // Temp for debug
    // else if (stateGamePlay->player->isFlying) {
    //   if (pressed.DpadUp) {
    //     stateGamePlay->player->flyUp(deltaTime);
    //   } else if (pressed.DpadDown) {
    //     stateGamePlay->player->flyDown(deltaTime);
    //   }
    // }

    // if (clicked.Cross) {
    //   if (elapsedTimeInSec < 0.45F) {
    //     stateGamePlay->player->toggleFlying();
    //   }
    //   elapsedTimeInSec = 0.0F;
    // }
    // Temp for debug
  }
}

u8 MazePlayingState::hasReachedTargetBlock() {
  return stateGamePlay->world->validTargetBlock() &&
         stateGamePlay->world->targetBlock->getType() ==
             Blocks::JACK_O_LANTERN_BLOCK;
}

void MazePlayingState::setHappyTheme() {
  stateGamePlay->world->dayNightCycleManager.resetSkyColor();
  tickManager.setDayMid();
  stateGamePlay->world->chunkManager.reloadLightDataOfAllChunks();
}

void MazePlayingState::setDarkTheme() {
  stateGamePlay->world->dayNightCycleManager.resetSkyColor();

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

  stateGamePlay->world->chunkManager.reloadLightDataOfAllChunks();
}

void MazePlayingState::navigate() {}

void MazePlayingState::renderMazeUi() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderUI == false) return;
#endif  // DEBUG_MODE

  stateGamePlay->ui->renderCrosshair();

  if (hasReachedTargetBlock()) {
    Renderer* t_renderer = &stateGamePlay->context->t_engine->renderer;

    t_renderer->renderer2D.render(btnL2);
    FontManager& fm = FontManager::getInstanceRef();

    FontOptions options;
    options.position.set(40, t_renderer->core.getSettings().getHeight() - 40);
    options.alignment = TextAlignment::Left;
    options.scale = 0.8F;
    fm.printText(Label_Interact, options);
  }
}

void MazePlayingState::drawDebugInfo() {
#ifdef DEBUG_MODE
  if (g_debug_menu.showDebugMenu) {
    renderDebugMenu();
    return;
  }
#endif  // end if DEBUG_MODE

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
  const Vec4 pos = *stateGamePlay->player->getPosition();
  std::string playerPosition =
      std::string("Player Position ")
          .append(" X: ")
          .append(std::to_string(static_cast<int>(pos.x / DOUBLE_BLOCK_SIZE)))
          .append("   Y: ")
          .append(std::to_string(static_cast<int>(pos.y / DOUBLE_BLOCK_SIZE)))
          .append("   Z: ")
          .append(std::to_string(static_cast<int>(pos.z / DOUBLE_BLOCK_SIZE)));
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

void MazePlayingState::printMemoryInfoToLog() {
  std::string freeRam =
      std::string("Free RAM: ")
          .append(std::to_string(
              stateGamePlay->context->t_engine->info.getAvailableRAM()))
          .append(" MB");
  TYRA_LOG(freeRam.c_str());
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
  mazeAudioListener.stopPlayingAll();

  saveProgress();

  NewGameOptions model = *stateGamePlay->world->getWorldOptions();
  model.seed += 1;

  TYRA_LOG("Generating new maze for seed: ", model.seed);
  stateGamePlay->world->setWorldOptions(model);
  stateGamePlay->world->setSeed(model.seed);

  TYRA_LOG("Generating world...");
  stateGamePlay->world->generate();

  delete stateGamePlay->world->targetBlock;
  stateGamePlay->world->targetBlock = nullptr;

  TYRA_LOG("Loading spawn area...");
  setDarkTheme();
  stateGamePlay->world->generateLight();
  stateGamePlay->world->generateSpawnArea();
  stateGamePlay->world->loadSpawnArea();

  TYRA_LOG("Resetting player position...");
  Vec4 spawnPos = stateGamePlay->world->getGlobalSpawnArea();
  stateGamePlay->player->setPosition(spawnPos);
  stateGamePlay->player->spawnArea.set(spawnPos);
  stateGamePlay->context->t_camera->setFirstPerson();

  stateGamePlay->world->setDrawDistanceMode(DrawDistanceMode::Low);

  shouldRenderLevelDoneDialog = false;
  shouldLoadNextLevel = false;
  _nextLevelCounter = 6.0f;

  TYRA_LOG("New level loaded.\n");
}

void MazePlayingState::renderCountDown() {
  FontManager& fm = FontManager::getInstanceRef();

  const float halfWidth = 512 / 2;
  stateGamePlay->context->t_engine->renderer.renderer2D.render(overlay);

  FontOptions titleOption;
  titleOption.position.set(halfWidth - 20, 110);
  titleOption.alignment = TextAlignment::Center;
  titleOption.scale = 1.6F;
  fm.printText(Label_LevelDone, titleOption);

  FontOptions textOption;
  textOption.position.set(halfWidth - 10, 185);
  textOption.alignment = TextAlignment::Center;
  textOption.scale = 0.8F;
  fm.printText(
      Label_LoadingNextLevelIn + std::to_string((int)_nextLevelCounter),
      textOption);
}