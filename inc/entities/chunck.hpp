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
  Vec4 scaledMinOffset = Vec4();
  Vec4 scaledMaxOffset = Vec4();
  Vec4* center = new Vec4();
  BBox* bbox;

  int visibleFacesCount = 0;
  u16 blocksCount = 0;

  LevelMap* terrain;
  WorldLightModel* t_worldLightModel;
  void init(LevelMap* terrain, WorldLightModel* t_worldLightModel);

  void renderer(Renderer* t_renderer, StaticPipeline* stapip,
                BlockManager* t_blockManager);
  void update(const Plane* frustumPlanes);
  void clear();

  void loadDrawData();
  void reloadLightData();
  void clearDrawData();
  inline const u8 isDrawDataLoaded() { return _isDrawDataLoaded; };

  CoreBBoxFrustum frustumCheck = CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  void updateFrustumCheck(const Plane* frustumPlanes);
  inline const u8 isVisible() {
    return this->frustumCheck != Tyra::CoreBBoxFrustum::OUTSIDE_FRUSTUM;
  }

  s8 getDistanceFromPlayerInChunks();
  void setDistanceFromPlayerInChunks(const s8 distante);

  // Block controllers
  void addBlock(Block* t_block);

  void preAllocateMemory();
  void freeUnusedMemory();
  bool isPreAllocated();

  Block* getBlockByPosition(const Vec4* pos);
  Block* getBlockByOffset(const Vec4* offset);

 private:
  std::vector<Vec4> vertices;
  std::vector<Color> verticesColors;
  std::vector<Vec4> uvMap;

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  void sortBlockByTransparency();

  inline void resetLoadingOffset() {
    this->tempLoadingOffset->set(*minOffset);
  };

  const Vec4* rawData;
  const Vec4* crossBlockRawData;

  u8 _isDrawDataLoaded = false;

  bool isBlockOpaque(u8 block_type);
  std::array<u8, 8> getFaceNeightbors(FACE_SIDE faceSide, Block* block);
  std::array<u8, 4> getCornersAOValues(std::array<u8, 8> blocksNeightbors);
  u8 getVertexAO(bool side1, bool corner, bool side2);
  float calcAOIntensity(u8 AOValue);

  void loadCuboidBlock(Block* t_block);
  void loadCrossBlock(Block* t_block);

  void loadMeshData(Block* t_block);
  void loadUVData(Block* t_block);
  void loadUVFaceData(const u8& index);
  void loadLightData(Block* t_block);

  void loadCrossedMeshData(Block* t_block);
  void loadCrossedUVData(Block* t_block);
  void loadCroosedLightData(Block* t_block);

  void loadLightFaceData(Color* faceColor);
  void loadLightFaceDataWithAO(Color* faceColor,
                               std::array<u8, 8>& faceNeightbors);

  s8 _distanceFromPlayerInChunks = -1;
  bool _isPreAllocated = false;
};
