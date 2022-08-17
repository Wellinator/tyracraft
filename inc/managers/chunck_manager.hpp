// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include <debug/debug.hpp>
#include "contants.hpp"
#include "entities/chunck.hpp"
#include "entities/player.hpp"
#include "managers/block_manager.hpp"
#include <math/vec4.hpp>
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "renderer/3d/bbox/bbox.hpp"
#include <math/m4x4.hpp>
#include <vector>

using Tyra::MinecraftPipeline;
using Tyra::Renderer;
using Tyra::Vec4;
using Tyra::M4x4;
using Tyra::BBox;

class ChunckManager {
 public:
  ChunckManager();
  ~ChunckManager();

  Chunck* getChunckByPosition(const Vec4& position);
  Chunck* getChunckById(const u16 id);
  std::vector<Chunck*> getChuncks() { return this->chuncks; };
  void init();
  void update(Player* t_player);
  void renderer(Renderer* t_renderer, MinecraftPipeline* t_mcPip,
                BlockManager* t_blockManager);

 private:
  std::vector<Chunck*> chuncks;

  void generateChunks();
};
