
#include "entities/mannequin/mannequin.hpp"

using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;

Mannequin::Mannequin() : Animated() {
  std::vector<u8> standStillSequence = {0, 1};
  AnimationOptions idleAnimation;
  idleAnimation.animationId = IDLE_ANIMATION;
  idleAnimation.framesIndices = standStillSequence;
  idleAnimation.durationInMs = 1000.0f;
  idleAnimation.loop = true;
  idleAnimation.wrapFrames = false;

  std::vector<u8> greetingSequence = {2, 3, 4, 5, 6, 5, 6, 5, 4, 3, 2};
  AnimationOptions greetingAnimation;
  greetingAnimation.animationId = GREETING_ANIMATION;
  greetingAnimation.framesIndices = greetingSequence;
  greetingAnimation.durationInMs = 2300.0f;
  greetingAnimation.loop = false;
  greetingAnimation.wrapFrames = false;

  addAnimation(idleAnimation);
  addAnimation(greetingAnimation);
  setAnimation(IDLE_ANIMATION);
}

Mannequin::~Mannequin() {}

void Mannequin::update(const float deltaTime) {
  Animated::update(deltaTime);

  if (greetingCooldownTimer <= 0.0F) {
    if (isIdleAnimation()) {
      setGreetingAnimation();
    }
    greetingCooldownTimer = Tyra::Math::randomf(5.0F, 15.0F);
  } else {
    greetingCooldownTimer -= deltaTime;
  }

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
  infoBag.blendingEnabled = false;
  infoBag.antiAliasingEnabled = false;
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

void Mannequin::onAnimationEnd() { setIdleAnimation(); }

void Mannequin::setIdleAnimation() { setAnimation(IDLE_ANIMATION); }

void Mannequin::setGreetingAnimation() { setAnimation(GREETING_ANIMATION); }

bool Mannequin::isIdleAnimation() {
  return getCurrentAnimation().animationId == IDLE_ANIMATION;
}
