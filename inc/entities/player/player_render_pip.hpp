#include <tyra>
#include <camera.hpp>

#pragma once

using Tyra::Renderer;

class Player;

class PlayerRenderPip {

  // remember last picked item
  protected:
  ItemId lastSelectedItem = ItemId::empty; 

  
 public:
  PlayerRenderPip(Player* t_player) { this->t_player = t_player; };

  virtual ~PlayerRenderPip(){};

  virtual void render(Renderer* t_render) = 0;
  virtual void update(const float& deltaTime, Camera* t_camera) = 0;
  virtual void loadItemDrawData() = 0;
  virtual void unloadItemDrawData() = 0;

  Player* t_player = nullptr;
};
