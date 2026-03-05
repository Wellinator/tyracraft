#include "managers/clouds_manager.hpp"
#include "managers/light_manager.hpp"
#include "managers/clipping_manager.hpp"
#include "debug.hpp"
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

  // Build the static parts of the persistent draw bag once
  _bagMatrix.identity();
  _bagTex.texture = cloudsTex;
  _bagTex.coordinates = uvMap;          // pointer stays valid (member array)
  _bagInfo.model = &_bagMatrix;
  _bagInfo.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  _bagInfo.blendingEnabled = true;
  _bagInfo.fullClipChecks = true;
  _bagInfo.frustumCulling = Tyra::PipelineInfoBagFrustumCulling::
      PipelineInfoBagFrustumCulling_Precise;
  _bagColor.single = &baseColor;
  _bag.count = DRAW_DATA_COUNT;
  _bag.vertices = vertices;             // pointer stays valid (member array)
  _bag.color = &_bagColor;
  _bag.info = &_bagInfo;
  _bag.texture = &_bagTex;
  _bagReady = true;
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
  lerpAcc = 0.0f;
  positionStart.set(position);
  positionEnd = position + (velocity * nextInteration);
};

void CloudsManager::update(const float deltaTime) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableClouds == false) return;
#endif  // DEBUG_MODE

  tempColor = LightManager::IntensifyColor(baseColor,
                                           worldLightModel->sunLightIntensity);

  lerpAcc += deltaTime;
  lerp = lerpAcc / nextInteration;
  Vec4::setLerp(&position, positionStart, positionEnd, lerp);

  if (position.x > 4.0F) position.x = 1;

  calcUVMapping();

  // Update dynamic parts of the persistent bag
  _bagColor.single = &baseColor;
  _bagTex.coordinates = uvMap;   // uvMap was just recalculated above
};

void CloudsManager::tick() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableClouds == false) return;
#endif  // DEBUG_MODE

  updateCloudsPosition();
};

void CloudsManager::render() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableClouds == false) return;
#endif  // DEBUG_MODE

  if (!_bagReady) return;

  t_renderer->renderer3D.usePipeline(stapip);
  stapip.core.render(&_bag);
};