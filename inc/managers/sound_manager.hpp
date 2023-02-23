#pragma once
#include <tamtypes.h>
#include <audsrv.h>
#include "tyra"
#include "constants.hpp"
#include "entities/sfx_library.hpp"

using Tyra::AdpcmResult;
using Tyra::Audio;
using Tyra::Engine;

class SoundManager {
 public:
  SoundManager(Engine* t_engine);
  ~SoundManager();

  SfxLibraryCategory* getCategory(const SoundFxCategory& idCategory) {
    return this->soundLibrary->getCategory(idCategory);
  }

  SfxLibrarySound* getSound(const SoundFxCategory& idCategory,
                            const SoundFX& idSound) {
    return this->soundLibrary->getSound(idCategory, idSound);
  }

  void playSfx(const SoundFxCategory& idCategory, const SoundFX& idSound);
  void playSfx(const SoundFxCategory& idCategory, const SoundFX& idSound,
               const s8& t_ch);

  void setSfxVolume(const u8& t_vol, const s8& t_ch);

  const int getAvailableChannel();

 private:
  /**
   * @brief Declaration of global current adpcm channel
   *
   */
  int currentAdpcmChannel = 0;

  SfxLibrary* soundLibrary;
  Engine* t_engine;
};
