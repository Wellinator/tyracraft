
#include "entities/mannequin/mannequin.hpp"

using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;

Mannequin::Mannequin() : Animated() {}

Mannequin::~Mannequin() {}

void Mannequin::update(const float deltaTime) {
  Animated::update(deltaTime);
  model.identity();
  model = translation * rotation * scale;
}

void Mannequin::render(StaticPipeline* pipeline, Texture* skinTexture) {
  vertices.clear();
  verticesColors.clear();
  uvMap.clear();

  // Calc draw data by frames interpolation
  fillDrawDataByLerp(&vertices, &verticesColors, &uvMap);

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  textureBag.coordinates = uvMap.data();
  textureBag.texture = skinTexture;

  infoBag.model = &model;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = true;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  Color tempColor = Color(128, 128, 128);
  colorBag.single = &tempColor;

  bag.count = vertices.size();
  bag.vertices = vertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  pipeline->core.render(&bag);
}
