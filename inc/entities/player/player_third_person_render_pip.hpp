#pragma once
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Renderer;

// TODO: fix player rotation
class PlayerThirdPersonRenderPip : public PlayerRenderPip {
 public:
  PlayerThirdPersonRenderPip(Player* t_player);
  ~PlayerThirdPersonRenderPip();

  void render(Renderer* t_render);
};
