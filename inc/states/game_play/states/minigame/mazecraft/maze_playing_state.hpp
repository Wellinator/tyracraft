#pragma once
#include "states/game_play/state_game_play.hpp"
#include "states/game_play/states/playing_state_base.hpp"
#include "states/game_play/states/minigame/mazecraft/maze_audio_listener.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/font/font_options.hpp"
#include "managers/tick_manager.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/inventory.hpp"
#include <tamtypes.h>
#include <string>
#include <tyra>

using Tyra::Pad;
using Tyra::PadButtons;
using Tyra::Threading;

class MazePlayingState : public PlayingStateBase {
 public:
  MazePlayingState(StateGamePlay* t_context);
  ~MazePlayingState();

  void init();
  void afterInit();
  void handleAction(MenuAction action);
  void update(const float& deltaTime);
  void tick();
  void render();

 private:
  void handleInput(const float& deltaTime);
  void navigate();
  void renderMazeUi();
  void drawDegubInfo();
  void playNewRandomSong();
  void gamePlayInputHandler(const float& deltaTime);
  void saveProgress();
  void loadNextLevel();

  u8 hasReachedTargetBlock();
  void setHappyTheme();
  void setDarkTheme();

  Sprite overlay;

  u8 shouldLoadNextLevel = false;
  u8 shouldRenderLevelDoneDialog = false;
  double _nextLevelCounter = 5.0f;
  void renderCountDown();
  const std::string Label_LevelDone =
      g_language_repository["minigame"]["common"]["level_done_exclamation"]
          .get<std::string>();
  const std::string Label_LoadingNextLevelIn =
      g_language_repository["minigame"]["common"]["loading_next_level_in"]
          .get<std::string>();

  inline const u8 isSongPlaying() {
    return mazeAudioListener.t_song->isPlaying();
  };

  /**
   * @brief Print RAM memory info to log
   *
   */
  void printMemoryInfoToLog();

  MazeAudioListener mazeAudioListener;
  u32 audioListenerId;
  // float elapsedTimeInSec;
  TickManager tickManager;

  Vec4 playerMovementDirection;
};
