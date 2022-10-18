
#include "states/game_play/creative_audio_listener.hpp"

CreativeAudioListener::CreativeAudioListener(Context* context) {
  this->t_context = context;
}

CreativeAudioListener::~CreativeAudioListener() {}

void CreativeAudioListener::onAudioTick() { }

void CreativeAudioListener::onAudioFinish() { this->playRandomCreativeSound(); }

void CreativeAudioListener::playRandomCreativeSound() {
  this->t_context->t_audio->song.stop();
  const std::string songName = this->getRandonMenuSongName();
  TYRA_LOG("Playing song -> ", songName.c_str());
  this->t_context->t_audio->song.load(FileUtils::fromCwd(songName));
  this->t_context->t_audio->song.inLoop = true;
  this->t_context->t_audio->song.setVolume(100);
  this->t_context->t_audio->song.play();
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
