#include "entities/player/player_render_body_pip.hpp"

using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;

PlayerRenderBodyPip::PlayerRenderBodyPip(Player* t_player)
    : PlayerRenderPip(t_player) {
  statPip.setRenderer(&t_player->t_renderer->core);
};

PlayerRenderBodyPip::~PlayerRenderBodyPip() {};

void PlayerRenderBodyPip::update(const float& deltaTime, Camera* t_camera) {
  baseColorAtPlayerPos = *t_player->getBaseColorAtPlayerPos();
};

void PlayerRenderBodyPip::render(Renderer* t_render) {
  std::vector<Vec4> vertices = {};
  std::vector<Color> verticesColors = {};
  std::vector<Vec4> uvMap = {};
  t_player->fillAnimationDrawData(&vertices, &verticesColors, &uvMap);

  StaPipTextureBag textureBag;
  textureBag.coordinates = uvMap.data();
  textureBag.texture = t_player->getPlayerTexture();

  StaPipInfoBag infoBag;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  StaPipColorBag colorBag;
  colorBag.single = &baseColorAtPlayerPos;

  M4x4 rawMatrix = M4x4::Identity;
  rawMatrix.rotate(t_player->rotation);
  rawMatrix.translate(t_player->position);
  infoBag.model = &rawMatrix;

  StaPipBag bag;
  bag.count = vertices.size();
  bag.vertices = vertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  t_render->renderer3D.usePipeline(statPip);
  statPip.core.render(&bag);
}

void PlayerRenderBodyPip::loadItemDrawData() {};

void PlayerRenderBodyPip::unloadItemDrawData() {};
