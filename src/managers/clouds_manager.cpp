#include "managers/clouds_manager.hpp"
#include "managers/light_manager.hpp"
#include "managers/tick_manager.hpp"
#include "managers/clipping_manager.hpp"
#include "math3d.h"
#include "camera.hpp"

CloudsManager::CloudsManager() {
  calcVertices();
  calcUVMapping();
}

CloudsManager::~CloudsManager() {
  t_renderer->getTextureRepository().free(cloudsTex->id);
}

void CloudsManager::init(Renderer* renderer,
                         WorldLightModel* t_worldLightModel) {
  t_renderer = renderer;
  worldLightModel = t_worldLightModel;
  stapip.setRenderer(&renderer->core);
  cloudsTex = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("/textures/environment/clouds.png"));
}

void CloudsManager::calcVertices() {
  Vec4 rawVertices[6]{
      Vec4(-1.0F, 1.0F, -1.0f), Vec4(1.0F, 1.0F, 1.0f),
      Vec4(1.0F, 1.0F, -1.0f),  Vec4(-1.0F, 1.0F, -1.0f),
      Vec4(-1.0F, 1.0F, 1.0f),  Vec4(1.0F, 1.0F, 1.0f),
  };

  M4x4 model;
  model.identity();
  model.scaleX(3000.0F);
  model.scaleZ(3000.0F);
  model.translateY(MAX_WORLD_POS.y - 100.0f);

  for (size_t i = 0; i < DRAW_DATA_COUNT; i++) {
    vertices[i] = model * rawVertices[i];
  }
}

void CloudsManager::calcUVMapping() {
  uvMap[0] = Vec4(position.x, (position.y + 1.0F), 1.0F) * scaleVec;
  uvMap[1] = Vec4((position.x + 1.0F), position.y, 1.0F) * scaleVec;
  uvMap[2] = Vec4((position.x + 1.0F), (position.y + 1.0F), 1.0F) * scaleVec;
  uvMap[3] = Vec4(position.x, (position.y + 1.0F), 1.0F) * scaleVec;
  uvMap[4] = Vec4(position.x, position.y, 1.0F) * scaleVec;
  uvMap[5] = Vec4((position.x + 1.0F), position.y, 1.0F) * scaleVec;
};

void CloudsManager::updateCloudsPosition() {
  positionStart.set(position);
  positionEnd = position + (velocity * TICKS_IN_SECONDS);
  lerp = 0.0f;
}

void CloudsManager::update(const float deltaTime) {
  tempColor = LightManager::IntensifyColor(baseColor,
                                           worldLightModel->sunLightIntensity);

  lerp += deltaTime / 0.05f;
  Vec4::setLerp(&position, positionStart, positionEnd, lerp);

  if (position.x > 4.0F) position.x = 1;
  if (position.y > 4.0F) position.y = 1;

  calcUVMapping();
};

void CloudsManager::tick() { updateCloudsPosition(); };

void CloudsManager::render() {
  t_renderer->renderer3D.usePipeline(stapip);

  M4x4 rawMatrix;
  rawMatrix.identity();

  StaPipTextureBag textureBag;
  textureBag.texture = cloudsTex;
  textureBag.coordinates = uvMap;

  StaPipInfoBag infoBag;
  infoBag.model = &rawMatrix;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.blendingEnabled = true;
  infoBag.fullClipChecks = true;
  infoBag.frustumCulling = Tyra::PipelineInfoBagFrustumCulling::
      PipelineInfoBagFrustumCulling_Precise;

  StaPipColorBag colorBag;
  colorBag.single = &baseColor;

  StaPipBag bag;
  bag.count = DRAW_DATA_COUNT;
  bag.vertices = vertices;
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  stapip.core.render(&bag);
};