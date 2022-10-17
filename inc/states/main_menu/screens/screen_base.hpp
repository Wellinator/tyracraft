#pragma once
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"

// Its context
class StateMainMenu;

class ScreenBase {
 public:
  ScreenBase(StateMainMenu* context) { this->context = context; };
  virtual ~ScreenBase(){};
  virtual void init() = 0;
  virtual void update() = 0;
  virtual void render() = 0;

  void playClickSound() {
    this->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                           SoundFX::Click);
  }

  StateMainMenu* context;
};