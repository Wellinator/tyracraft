#pragma once

// Its context
class StateGamePlay;

class PlayingStateBase {
 public:
  PlayingStateBase(StateGamePlay* context) { this->stateGamePlay = context; };
  virtual ~PlayingStateBase(){};
  virtual void init() = 0;
  virtual void update(const float& deltaTime) = 0;
  virtual void render() = 0;
  virtual void handleInput(const float& deltaTime) = 0;
  StateGamePlay* stateGamePlay;
};