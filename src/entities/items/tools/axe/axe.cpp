#pragma once
#include "entities/items/tools/axe/axe.hpp"
#include <tyra>

using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;

Axe::Axe(ItemsMaterials material) {}
Axe::~Axe() {}

void Axe::update() {}
void Axe::init(Renderer* t_renderer) {
  this->t_renderer = t_renderer;
  ObjLoader loader;

  ObjLoaderOptions objOptions;
  objOptions.flipUVs = true;
  objOptions.scale = .5F;

  auto data =
      loader.load(FileUtils::fromCwd("meshes/tools/axe/axe.obj"), objOptions);

  data->loadNormals = false;
  this->mesh = new StaticMesh(data.get());

  mesh->rotation.rotateY(1.60F);
  mesh->rotation.rotateZ(-6.30F);
  mesh->scale.scale(0.5F);

  t_renderer->core.texture.repository->addByMesh(
      // TODO: load texture by material
      mesh, FileUtils::fromCwd("meshes/tools/axe/wooden/"), "png");
}

void Axe::allocateOptions() {
  options = new StaPipOptions();
  options->textureMappingType = Tyra::TyraNearest;
  options->shadingType = Tyra::TyraShadingFlat;
  options->blendingEnabled = false;
  options->fullClipChecks = false;
  options->frustumCulling = Tyra::PipelineFrustumCulling_None;
  options->transformationType = Tyra::TyraMP;
  options->antiAliasingEnabled = false;
}