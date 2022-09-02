#pragma once
#include "entities/items/tools/tool.hpp"
#include "entities/items/materials.hpp"
#include <tyra>

using Tyra::Renderer;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;

class Axe : public Tool {
 public:
  Axe(ItemsMaterials material);
  ~Axe();

  std::unique_ptr<StaticMesh> mesh;
  StaPipOptions* options;

  void init(Renderer* t_renderer);
  void update();

 private:
  Renderer* t_renderer;

  void allocateOptions();
};