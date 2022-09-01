#pragma once
#include "entities/items/tools/tool.hpp"
#include "entities/items/materials.hpp"
#include <tyra>

using Tyra::Renderer;
using Tyra::StaPipOptions;
using Tyra::StaticMesh;

class Axe : public Tool {
  Axe(ItemsMaterials material);
  ~Axe();

 public:
  StaticMesh* mesh;

  void update();

 private:
  Renderer* t_renderer;
  StaPipOptions* options;

  void init(Renderer* t_renderer);
  void playBreakSound();
  void allocateOptions();
};