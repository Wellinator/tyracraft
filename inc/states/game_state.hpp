#pragma once
#include "managers/state_manager.hpp"

class GameState {
 public:
  GameState();
  ~GameState();
  virtual void init() = 0;
  virtual void update() = 0;
  virtual void render() = 0;

 protected:
  StateManager* context;
};