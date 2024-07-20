
#pragma once
#include <tamtypes.h>
#include <tyra>
#include <audio/audio_listener.hpp>
#include "constants.hpp"

using Tyra::AudioListener;
using Tyra::AudioSong;
using Tyra::FileUtils;
using Tyra::Math;

class MazeAudioListener : public AudioListener {
 public:
  MazeAudioListener();
  ~MazeAudioListener();

  void init(AudioSong* audioSong);
  void onAudioTick();
  void onAudioFinish();
  void playRandomMazeSound();
  void playLevelDoneSound();
  const std::string getRandonMenuSongName();

  AudioSong* t_song = nullptr;
};
