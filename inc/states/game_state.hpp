#pragma once
#include "states/context.hpp"

class Context;
class GameState {
 public:
  GameState(Context* t_context) { this->context = t_context; };
  virtual ~GameState(){};
  virtual void init() = 0;
  virtual void afterInit() = 0;
  virtual void update(const float& deltaTime) = 0;
  virtual void render() = 0;

  Context* context;
};