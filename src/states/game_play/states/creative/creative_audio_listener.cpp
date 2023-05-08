
#include "states/game_play/states/creative/creative_audio_listener.hpp"
#include "managers/sound_manager.hpp"

CreativeAudioListener::CreativeAudioListener() {}

CreativeAudioListener::~CreativeAudioListener() {}

void CreativeAudioListener::init(AudioSong* audioSong) { t_song = audioSong; }

void CreativeAudioListener::onAudioTick() {}

void CreativeAudioListener::onAudioFinish() {}

void CreativeAudioListener::playRandomCreativeSound() {
  t_song->stop();
  const std::string sonfToPlay = SoundManager::GetRandonSongFromPath(
      FileUtils::fromCwd("sounds/game/creative/"));

  if (sonfToPlay.size() > 0) {
    TYRA_LOG("Playing song -> ", sonfToPlay.c_str());
    t_song->load(sonfToPlay);
    t_song->inLoop = false;
    t_song->setVolume(90);
    t_song->play();
  }
}
