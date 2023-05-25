#pragma once

#include <tamtypes.h>
#include <tyra>
#include <audsrv.h>

class SfxConfigModel {
 public:
  SfxConfigModel() {}

  SfxConfigModel(u8 pitch, u8 volume) {
    _pitch = pitch;
    _volume = volume;
  };

  ~SfxConfigModel(){};

  u8 _pitch;
  u8 _volume;
};
