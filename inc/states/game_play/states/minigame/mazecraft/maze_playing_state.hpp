#pragma once

#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/states/minigame/mazecraft/maze_audio_listener.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/tick_manager.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/inventory.hpp"
#include <tamtypes.h>
#include <string>
#include <tyra>

using Tyra::Pad;
using Tyra::PadButtons;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::TextureRepository;
using Tyra::Threading;
using Tyra::Vec4;

class MazePlayingState : public PlayingStateBase {
 public:
  MazePlayingState(StateGamePlay* t_context);
  ~MazePlayingState();

  void init();
  void afterInit();
  void handleAction(MenuAction action);
  void fixedUpdate(const float& fixedDeltaTime);
  void update(const float& deltaTime);
  void tick();
  void render();

 private:
  void handleInput(const float& deltaTime);
  void navigate();
  void renderMazeUi();
  void drawDegubInfo();
  void gamePlayInputHandler(const float& deltaTime);
  void saveProgress();
  void loadNextLevel();

  u8 hasReachedTargetBlock();
  void setHappyTheme();
  void setDarkTheme();

  Sprite overlay;
  Sprite btnL2;

  u8 shouldLoadNextLevel = false;
  u8 shouldRenderLevelDoneDialog = false;
  double _nextLevelCounter = 6.0f;
  void renderCountDown();

  const std::string Label_LevelDone =
      LanguageManager::Translate("/minigame/common/level_done_exclamation");
  const std::string Label_LoadingNextLevelIn =
      LanguageManager::Translate("/minigame/common/loading_next_level_in");
  const std::string Label_Interact =
      LanguageManager::Translate("/minigame/common/interact");

  inline const u8 isSongPlaying() {
    return mazeAudioListener.t_song->isPlaying();
  };

  /**
   * @brief Print RAM memory info to log
   *
   */
  void printMemoryInfoToLog();

  PostFxManager postFxManager;

  MazeAudioListener mazeAudioListener;
  u32 audioListenerId;
  float elapsedTimeInSec;
  TickManager tickManager;

  Vec4 playerMovementDirection;
};
