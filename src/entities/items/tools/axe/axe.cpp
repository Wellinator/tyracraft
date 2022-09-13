#pragma once
#include "entities/items/tools/axe/axe.hpp"
#include "entities/items/tools/tool.hpp"
#include "entities/items/materials.hpp"
#include <tyra>

using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;

Axe::Axe(ItemsMaterials material) {}

Axe::~Axe() {
  this->t_renderer->getTextureRepository().freeByMesh(this->mesh.get());
}

void Axe::update() {}
void Axe::init(Renderer* t_renderer) {
  this->t_renderer = t_renderer;
  this->allocateOptions();

  ObjLoaderOptions objOptions;
  objOptions.flipUVs = true;
  objOptions.scale = 1.5F;

  auto data = ObjLoader::load(FileUtils::fromCwd("meshes/tools/axe/axe.obj"),
                              objOptions);

  data.get()->loadNormals = false;
  this->mesh = std::make_unique<StaticMesh>(data.get());

  mesh.get()->translation.identity();
  mesh.get()->translation.translateX(2.85F);
  mesh.get()->translation.translateY(-3.40F);
  mesh.get()->translation.translateZ(-10.50F);

  // mesh.get()->rotation.identity();
  mesh.get()->rotation.rotateY(1.60F);
  // mesh.get()->rotation.rotateZ(1.5708F);

  mesh.get()->scale.identity();
  mesh.get()->scale.scale(200.0F);

  // TODO: load texture by material
  t_renderer->getTextureRepository().addByMesh(
      mesh.get(), FileUtils::fromCwd("meshes/tools/axe/wooden/"), "png");
}

void Axe::allocateOptions() {
  options = new StaPipOptions();
  options->textureMappingType = Tyra::TyraNearest;
  options->shadingType = Tyra::TyraShadingFlat;
  options->blendingEnabled = false;
  options->fullClipChecks = false;
  options->frustumCulling = Tyra::PipelineFrustumCulling_None;
  options->transformationType = Tyra::TyraMVP;
  options->antiAliasingEnabled = false;
}