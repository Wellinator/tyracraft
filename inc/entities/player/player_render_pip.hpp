#include <tyra>

#pragma once

using Tyra::Renderer;

class Player;

class PlayerRenderPip {
 public:
  PlayerRenderPip(Player* t_player) { this->t_player = t_player; };

  virtual ~PlayerRenderPip(){};

  virtual void render(Renderer* t_render) = 0;
  virtual void loadItemDrawData() = 0;
  virtual void unloadItemDrawData() = 0;

  Player* t_player = nullptr;
};