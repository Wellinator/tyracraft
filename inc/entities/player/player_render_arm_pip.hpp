#pragma once
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::M4x4;
using Tyra::Renderer;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;

class PlayerRenderArmPip : public PlayerRenderPip {
 public:
  PlayerRenderArmPip(Player* t_player);
  ~PlayerRenderArmPip();

  void render(Renderer* t_render);
  void renderArm(Renderer* t_render);
  void renderItem(Renderer* t_render);

 private:
  std::vector<Vec4> vertices;
  std::vector<Color> verticesColors;
  std::vector<Vec4> uvMap;

  StaticPipeline stapip;
  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  M4x4 rawMatrix = M4x4::Identity;

  void loadItemDrawData();
  void unloadItemDrawData();
};
