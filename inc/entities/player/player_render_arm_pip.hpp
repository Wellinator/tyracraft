#pragma once
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Renderer;

class PlayerRenderArmPip : public PlayerRenderPip {
 public:
  PlayerRenderArmPip(Player* t_player);
  ~PlayerRenderArmPip();

  void render(Renderer* t_render);
};
