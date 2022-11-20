
#pragma once
#include <tamtypes.h>
#include <tyra>
#include <audio/audio_listener.hpp>
#include "constants.hpp"

using Tyra::AudioListener;
using Tyra::AudioSong;
using Tyra::FileUtils;
using Tyra::Math;

class CreativeAudioListener : public AudioListener {
 public:
  CreativeAudioListener(AudioSong* t_song);
  ~CreativeAudioListener();

  void onAudioTick();
  void onAudioFinish();
  void playRandomCreativeSound();
  const std::string getRandonMenuSongName();

  AudioSong* t_song = nullptr;
};
