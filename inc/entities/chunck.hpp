#pragma once

#include <vector>
#include <math/vec4.hpp>
#include <renderer/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
#include "entities/Block.hpp"
#include "entities/player.hpp"
#include "contants.hpp"
#include "managers/block_manager.hpp"
#include "utils.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"

using Tyra::MinecraftPipeline;
using Tyra::Renderer;

class Chunck {
 public:
  Chunck(BlockManager* t_blockManager);
  ~Chunck();

  std::vector<Block> blocks;

  inline int getChunckSize() const {
    return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE;
  };

  void renderer(Renderer* t_renderer, MinecraftPipeline* mcPip);
  void update(Player* t_player);
  void clear();
  void setToChanged();

  // Block controllers
  void addBlock(Block* t_block);

 private:
  BlockManager* blockManager;
  MinecraftPipeline mcPip;
  u8 hasChanged;
  std::vector<Block> singleTexBlocks;
  std::vector<Block> multiTexBlocks;

  float getVisibityByPosition(float d);
  void applyFOG(Block* t_block, const Vec4& originPosition);
  void highLightTargetBlock(Block* t_block, u8& isTarget);
  void updateBlocks(const Vec4& playerPosition);
  void filterSingleAndMultiBlocks();
};
