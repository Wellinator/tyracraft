#pragma once

// Its context
class StateMainMenu;

class ScreenBase {
 public:
  ScreenBase(StateMainMenu* context) { this->context = context; };
  virtual ~ScreenBase(){};
  virtual void init() = 0;
  virtual void update() = 0;
  virtual void render() = 0;
  StateMainMenu* context;
};