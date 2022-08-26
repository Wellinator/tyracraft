#pragma once
#include "states/context.hpp"

class GameState {
 public:
  GameState(Context* t_context) { this->context = t_context; };
  ~GameState(){};
  virtual void init() = 0;
  virtual void update() = 0;
  virtual void render() = 0;

  Context* context;
};