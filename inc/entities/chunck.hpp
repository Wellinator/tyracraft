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
#include "managers/block/vertex_block_data.hpp"
#include <math/m4x4.hpp>
#include "models/world_light_model.hpp"

using Tyra::BBox;
using Tyra::BBoxFace;
using Tyra::Color;
using Tyra::CoreBBoxFrustum;
using Tyra::M4x4;
using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::PipelineDirLightsBag;
using Tyra::Plane;
using Tyra::Renderer;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipLightingBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;
using Tyra::Vec4;

enum class ChunkState { Loaded, Clean };

class Chunck {
 public:
  Chunck(const Vec4& minOffset, const Vec4& maxOffset, u16 id);
  ~Chunck();

  u16 id = 0;

  ChunkState state = ChunkState::Clean;

  std::vector<Block*> blocks;
  Vec4* tempLoadingOffset = new Vec4();
  Vec4* minOffset = new Vec4();
  Vec4* maxOffset = new Vec4();
  Vec4* center = new Vec4();
  BBox* bbox;
  CoreBBoxFrustum frustumCheck = CoreBBoxFrustum::OUTSIDE_FRUSTUM;

  void renderer(Renderer* t_renderer, StaticPipeline* stapip,
                BlockManager* t_blockManager);
  void update(const Plane* frustumPlanes, const Vec4& currentPlayerPos,
              WorldLightModel* worldLightModel);
  void clear();
  void updateDrawData();
  void updateFrustumCheck(const Plane* frustumPlanes);

  inline const u8 isVisible() {
    return this->frustumCheck != Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  }

  // Block controllers
  void addBlock(Block* t_block);

  inline std::vector<Vec4> getVertexData() { return vertices; }

  inline std::vector<Color> getVertexColorData() { return verticesColors; }

 private:
  std::vector<Vec4> vertices;
  std::vector<Color> verticesColors;
  std::vector<Vec4> verticesNormals;
  std::vector<Vec4> uvMap;

  float getVisibityByPosition(float d);
  void applyFOG(const Vec4& originPosition);
  void highLightTargetBlock(Block* t_block, u8& isTarget);
  void updateBlocks(const Vec4& playerPosition);
  void filterSingleAndMultiBlocks();

  inline void resetLoadingOffset() {
    this->tempLoadingOffset->set(*minOffset);
  };

  void loadBags();
  void clearDrawData();
  void deallocDrawBags(StaPipBag* bag);
  StaPipBag* getDrawData();

  inline const bool hasDataToDraw() { return vertices.size() > 0; };

  VertexBlockData vertexBlockData;

  Vec4 sunPosition;
  float sunLightIntensity;
  float ambientLightIntesity;
};
