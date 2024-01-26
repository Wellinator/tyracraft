#pragma once
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Renderer;

// TODO: fix player rotation
class PlayerRenderBodyPip : public PlayerRenderPip {
 public:
  PlayerRenderBodyPip(Player* t_player);
  ~PlayerRenderBodyPip();

  void update(const float& deltaTime, Camera* t_camera){};
  void render(Renderer* t_render);
  void loadItemDrawData();
  void unloadItemDrawData();
};
