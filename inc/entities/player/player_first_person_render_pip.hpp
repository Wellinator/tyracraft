#pragma once
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Renderer;

class PlayerFirstPersonRenderPip : public PlayerRenderPip {
 public:
  PlayerFirstPersonRenderPip(Player* t_player);
  ~PlayerFirstPersonRenderPip();

  void render(Renderer* t_render);
};
