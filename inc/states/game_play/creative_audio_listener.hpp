
#pragma once
#include <tamtypes.h>
#include <tyra>
#include <audio/audio_listener.hpp>
#include "contants.hpp"
#include "states/context.hpp"

using Tyra::Audio;
using Tyra::AudioListener;
using Tyra::FileUtils;
using Tyra::Math;

class CreativeAudioListener : public AudioListener {
 public:
  CreativeAudioListener(Context* t_context);
  ~CreativeAudioListener();

  void onAudioTick();
  void onAudioFinish();
  void playRandomCreativeSound();
  const std::string getRandonMenuSongName();

  Context* t_context;
};
