#pragma once

#include <vector>
#include <math/vec4.hpp>
#include <renderer/renderer.hpp>
#include <fastmath.h>
#include <algorithm>
#include "entities/Block.hpp"
#include "constants.hpp"
#include "managers/block_manager.hpp"
#include "utils.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "renderer/3d/bbox/bbox.hpp"
#include <math/m4x4.hpp>

using Tyra::BBox;
using Tyra::CoreBBoxFrustum;
using Tyra::M4x4;
using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::Vec4;

enum class ChunkState { Loaded, Clean };

class Chunck {
 public:
  Chunck(const Vec4& minOffset, const Vec4& maxOffset, u16 id);
  ~Chunck();

  u16 id = 0;

  ChunkState state = ChunkState::Clean;

  std::vector<Block*> blocks;
  Vec4* minOffset = new Vec4();
  Vec4* maxOffset = new Vec4();
  Vec4* center = new Vec4();
  BBox* bbox;
  CoreBBoxFrustum frustumCheck = CoreBBoxFrustum::OUTSIDE_FRUSTUM;

  void renderer(Renderer* t_renderer, MinecraftPipeline* mcPip,
                BlockManager* t_blockManager);
  void update(const Plane* frustumPlanes);
  void clear();
  void updateDrawData();
  void updateFrustumCheck(const Plane* frustumPlanes);
  u8 isVisible();

  // Block controllers
  void addBlock(Block* t_block);

 private:
  std::vector<McpipBlock*> singleTexBlocks;
  std::vector<McpipBlock*> multiTexBlocks;

  float getVisibityByPosition(float d);
  void applyFOG(Block* t_block, const Vec4& originPosition);
  void highLightTargetBlock(Block* t_block, u8& isTarget);
  void updateBlocks(const Vec4& playerPosition);
  void filterSingleAndMultiBlocks();
  void clearMcpipBlocks();
};
