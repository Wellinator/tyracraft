
#include "states/game_play/states/minigame/mazecraft/maze_audio_listener.hpp"
#include "managers/sound_manager.hpp"

MazeAudioListener::MazeAudioListener() {}

MazeAudioListener::~MazeAudioListener() {}

void MazeAudioListener::init(AudioSong* audioSong) { t_song = audioSong; }

void MazeAudioListener::onAudioTick() {}

void MazeAudioListener::onAudioFinish() {}

void MazeAudioListener::playRandomMazeSound() {
  TYRA_LOG("Picking a random maze sound...");
  t_song->stop();
  const std::string soundToPlay = SoundManager::GetRandonSongFromPath(
      FileUtils::fromCwd("sounds/game/mazecraft/"));

  if (soundToPlay.empty()) return;

  TYRA_LOG("Playing song -> ", soundToPlay.c_str());
  t_song->load(soundToPlay);
  t_song->inLoop = false;
  t_song->setVolume(90);
  t_song->play();
}

void MazeAudioListener::playLevelDoneSound() {
  t_song->stop();
  const std::string levelDoneSfx =
      FileUtils::fromCwd("sounds/game/mazecraft/level_done.wav");

  if (levelDoneSfx.size() > 0) {
    TYRA_LOG("Playing song -> ", levelDoneSfx.c_str());
    t_song->load(levelDoneSfx);
    t_song->inLoop = false;
    t_song->setVolume(90);
    t_song->play();
  }
}

void MazeAudioListener::stopPlayingAll() {
  t_song->stop();
  t_song->setVolume(0);
}
