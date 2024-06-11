
#include "states/game_play/states/maze/maze_audio_listener.hpp"
#include "managers/sound_manager.hpp"

MazeAudioListener::MazeAudioListener() {}

MazeAudioListener::~MazeAudioListener() {}

void MazeAudioListener::init(AudioSong* audioSong) { t_song = audioSong; }

void MazeAudioListener::onAudioTick() {}

void MazeAudioListener::onAudioFinish() {}

void MazeAudioListener::playRandomMazeSound() {
  t_song->stop();
  const std::string sonfToPlay = SoundManager::GetRandonSongFromPath(
      FileUtils::fromCwd("sounds/game/maze/"));

  if (sonfToPlay.size() > 0) {
    TYRA_LOG("Playing song -> ", sonfToPlay.c_str());
    t_song->load(sonfToPlay);
    t_song->inLoop = false;
    t_song->setVolume(65);
    t_song->play();
  }
}
