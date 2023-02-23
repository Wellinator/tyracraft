
#include "states/game_play/states/creative/creative_audio_listener.hpp"

CreativeAudioListener::CreativeAudioListener() {}

CreativeAudioListener::~CreativeAudioListener() {}

void CreativeAudioListener::init(AudioSong* audioSong) { t_song = audioSong; }

void CreativeAudioListener::onAudioTick() {}

void CreativeAudioListener::onAudioFinish() {}

void CreativeAudioListener::playRandomCreativeSound() {
  t_song->stop();
  const std::string songName = getRandonMenuSongName();
  TYRA_LOG("Playing song -> ", songName.c_str());
  t_song->load(FileUtils::fromCwd(songName));
  t_song->inLoop = false;
  t_song->setVolume(90);
  t_song->play();
}

const std::string CreativeAudioListener::getRandonMenuSongName() {
  std::string result = "sounds/game/creative/creative";
  std::string songExtension = ".wav";
  const int numberOfSongs = 6;

  int randSongId = Math::randomi(1, numberOfSongs);
  result.append(std::to_string(randSongId));
  result.append(songExtension);
  return result;
}
