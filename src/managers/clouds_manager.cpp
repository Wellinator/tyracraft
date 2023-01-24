#include "managers/clouds_manager.hpp"

CloudsManager::CloudsManager() {
  const float scale = 1.0F / 4.0F;
  const Vec4& scaleVec = Vec4(scale, scale, 1.0F, 0.0F);
  const u8 X = 1;
  const u8 Y = 1;

  uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((X + 1.0F), (Y + 1.0F), 1.0F, 0.0F) * scaleVec);

  uvMap.push_back(Vec4(X, (Y + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4(X, Y, 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((X + 1.0F), Y, 1.0F, 0.0F) * scaleVec);
}

CloudsManager::~CloudsManager() {}

void CloudsManager::init(Renderer* renderer) {
  t_renderer = renderer;
  stapip.setRenderer(&renderer->core);
  cloudsTex = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("/textures/environment/clouds.png"));
}

void CloudsManager::update(){};

void CloudsManager::render() {
  t_renderer->renderer3D.usePipeline(stapip);

  M4x4 rawMatrix;
  rawMatrix.identity();
//   rawMatrix.rotateX(1.57F);
  rawMatrix.scale(100.0F);
  rawMatrix.translateY(-50.0F);

  StaPipTextureBag textureBag;
  textureBag.texture = cloudsTex;
  textureBag.coordinates = uvMap.data();

  StaPipInfoBag infoBag;
  infoBag.model = &rawMatrix;
  infoBag.blendingEnabled = true;
  infoBag.shadingType = Tyra::TyraShadingFlat;
  infoBag.textureMappingType = Tyra::TyraNearest;
  infoBag.transformationType = Tyra::TyraMP;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  // Apply multiple colors
  StaPipColorBag colorBag;
  Color baseColor = Color(128.0F, 128.0F, 128.0F);
  colorBag.single = &baseColor;

  StaPipBag bag;
  bag.count = vertices.size();
  bag.vertices = vertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  stapip.core.render(&bag);
};