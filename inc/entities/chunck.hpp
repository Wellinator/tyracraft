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
#include "entities/level.hpp"
#include <array>

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

enum class ChunkState { PreLoaded, Loaded, Loading, Clean };

class Chunck {
 public:
  Chunck(const Vec4& minOffset, const Vec4& maxOffset, const u16& id);
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

  int visibleFacesCount = 0;

  void renderer(Renderer* t_renderer, StaticPipeline* stapip,
                BlockManager* t_blockManager);
  void update(const Plane* frustumPlanes, const Vec4& currentPlayerPos);
  void clear();
  void updateFrustumCheck(const Plane* frustumPlanes);

  void loadDrawData(LevelMap* terrain, WorldLightModel* t_worldLightModel);
  void reloadLightData(LevelMap* terrain, WorldLightModel* t_worldLightModel);
  void clearDrawData();
  inline const u8 isDrawDataLoaded() { return _isDrawDataLoaded; };

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
  std::vector<Vec4> uvMap;

  float getVisibityByPosition(float d);
  void applyFOG(const Vec4& originPosition);
  void updateBlocks(const Vec4& playerPosition);
  void filterSingleAndMultiBlocks();
  void sortBlockByTransparency();

  inline void resetLoadingOffset() {
    this->tempLoadingOffset->set(*minOffset);
  };

  void deallocDrawBags(StaPipBag* bag);
  StaPipBag* getDrawData();

  inline const bool hasDataToDraw() { return vertices.size() > 0; };

  Vec4 sunPosition;
  float sunLightIntensity;
  float ambientLightIntesity;

  u8 _isDrawDataLoaded = false;

  bool isBlockOpaque(u8 block_type);
  std::array<u8, 8> getFaceNeightbors(LevelMap* terrain, FACE_SIDE faceSide,
                                      Block* block);
  std::array<u8, 4> getCornersAOValues(std::array<u8, 8> blocksNeightbors);
  u8 getVertexAO(bool side1, bool corner, bool side2);
  float calcAOIntensity(u8 AOValue);

  void loadMeshData(Block* t_block);
  void loadUVData(Block* t_block);
  void loadUVFaceData(const u8& X, const u8& Y);
  void loadLightData(LevelMap* terrain, Block* t_block);
  void loadLightFaceData(Color* faceColor);
  void loadLightFaceDataWithAO(Color* faceColor,
                               std::array<u8, 8>& faceNeightbors);
};
