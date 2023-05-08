#include "managers/clouds_manager.hpp"

CloudsManager::CloudsManager() { calcUVMapping(); }

CloudsManager::~CloudsManager() {
  t_renderer->getTextureRepository().free(cloudsTex->id);
}

void CloudsManager::init(Renderer* renderer) {
  t_renderer = renderer;
  stapip.setRenderer(&renderer->core);
  cloudsTex = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("/textures/environment/clouds.png"));
}

void CloudsManager::calcUVMapping() {
  uvMap.clear();
  uvMap.shrink_to_fit();

  const float scale = 1.0F / 4.0F;
  const Vec4& scaleVec = Vec4(scale, scale, 1.0F, 0.0F);

  uvMap.push_back(Vec4(XMap, (YMap + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((XMap + 1.0F), YMap, 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((XMap + 1.0F), (YMap + 1.0F), 1.0F, 0.0F) * scaleVec);

  uvMap.push_back(Vec4(XMap, (YMap + 1.0F), 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4(XMap, YMap, 1.0F, 0.0F) * scaleVec);
  uvMap.push_back(Vec4((XMap + 1.0F), YMap, 1.0F, 0.0F) * scaleVec);
};

void CloudsManager::updateCloudsPosition() {
  XMap += 0.00001F;
  YMap += 0.00001F;
  if (XMap > 4.0F) XMap = 1;
  if (YMap > 4.0F) YMap = 1;
  calcUVMapping();
}

void CloudsManager::update() { updateCloudsPosition(); };

void CloudsManager::render() {
  t_renderer->renderer3D.usePipeline(stapip);

  M4x4 rawMatrix;
  rawMatrix.identity();

  rawMatrix.scaleX(3000.0F);
  rawMatrix.scaleZ(3000.0F);
  rawMatrix.translateY(MAX_WORLD_POS.y);

  StaPipTextureBag textureBag;
  textureBag.texture = cloudsTex;
  textureBag.coordinates = uvMap.data();

  StaPipInfoBag infoBag;
  infoBag.model = &rawMatrix;
  infoBag.textureMappingType = Tyra::TyraNearest;
  infoBag.fullClipChecks = true;
  infoBag.frustumCulling = Tyra::PipelineInfoBagFrustumCulling::
      PipelineInfoBagFrustumCulling_Precise;

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

  // // Render sky limits || Debug content
  // {
  //   t_renderer->renderer3D.utility.drawLine(
  //       rawMatrix * vertices[0], rawMatrix * vertices[1], Color(255, 0, 0));
  //   t_renderer->renderer3D.utility.drawLine(
  //       rawMatrix * vertices[1], rawMatrix * vertices[2], Color(255, 0, 0));
  //   t_renderer->renderer3D.utility.drawLine(
  //       rawMatrix * vertices[2], rawMatrix * vertices[0], Color(255, 0, 0));

  //   t_renderer->renderer3D.utility.drawLine(
  //       rawMatrix * vertices[3], rawMatrix * vertices[4], Color(255, 0, 0));
  //   t_renderer->renderer3D.utility.drawLine(
  //       rawMatrix * vertices[4], rawMatrix * vertices[5], Color(255, 0, 0));
  //   t_renderer->renderer3D.utility.drawLine(
  //       rawMatrix * vertices[5], rawMatrix * vertices[3], Color(255, 0, 0));
  // }
};