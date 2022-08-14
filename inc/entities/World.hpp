#pragma once

#include <engine.hpp>
#include <tamtypes.h>
#include <time/timer.hpp>
#include <pad/pad.hpp>
#include <renderer/renderer.hpp>
#include "chunck.hpp"
#include "entities/player.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/items_repository.hpp"
#include "contants.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"

using Tyra::MinecraftPipeline;
using Tyra::Renderer;

class World {
 public:
  World();
  ~World();

  Renderer* t_renderer;
  TerrainManager* terrainManager = new TerrainManager();

  void init(Renderer* t_renderer, ItemRepository* itemRepository);
  void update(Player* t_player, Camera* t_camera, Pad* t_pad);
  void render();

 private:
  MinecraftPipeline mcPip;
  std::vector<Chunck*> chuncks ;

  void generateChunks();
};
