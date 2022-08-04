#pragma once

#include <tamtypes.h>
#include <engine.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/3d/bbox/core_bbox.hpp>
#include <renderer/3d/bbox/bbox.hpp>
#include <math/vec4.hpp>
#include <physics/ray.hpp>
#include <fastmath.h>
#include <draw_sampling.h>
#include "contants.hpp"
#include "entities/Block.hpp"
#include "entities/chunck.hpp"
#include "entities/player.hpp"
#include "entities/item.hpp"
#include "3libs/FastNoiseLite/FastNoiseLite.h"
#include "managers/block_manager.hpp"
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "managers/items_repository.hpp"
#include "debug/debug.hpp"

using Tyra::BBox;
using Tyra::Engine;
using Tyra::MinecraftPipeline;
using Tyra::Renderer;
using Tyra::Vec4;

class TerrainManager {
 public:
  TerrainManager();
  ~TerrainManager();
  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            MinecraftPipeline* mcPip);
  void update(Player* t_player, Camera* t_camera, Pad* t_pad);
  void generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees,
                          bool makeWater, bool makeCaves);
  inline Chunck* getChunck() { return this->chunck; };
  Chunck* getChunck(int offsetX, int offsetY, int offsetZ);
  void updateChunkByPlayerPosition(Player* player);

  Block* targetBlock = NULL;
  void getTargetBlock(const Vec4& playerPosition, Camera* t_camera);
  void removeBlock();
  void putBlock(u8 blockType);

  Renderer* t_renderer;
  Engine* engine;

  Vec4 worldSpawnArea;
  Vec4 spawnArea;
  void defineSpawnArea();
  const Vec4 calcSpawOffset(int bias = 0);

  int getNoise(int x, int z);

 private:
  Ray ray;
  u8 shouldUpdateChunck = 0;

  Player* t_player;
  Camera* t_camera;
  ItemRepository* t_itemRepository;

  // TODO: Refactor to region and cache it. See
  // https://minecraft.fandom.com/el/wiki/Region_file_format;
  Chunck* chunck;
  u8* terrain = new u8[OVERWORLD_SIZE];
  Vec4 minWorldPos;
  Vec4 maxWorldPos;

  Vec4* lastPlayerPosition = NULL;
  int blockToRemoveIndex;
  int blockToPlaceIndex;

  BlockManager* blockManager = new BlockManager();

  // Params for noise generation;
  const float frequency = 0.01;
  const float amplitude = 0.65f;
  const float lacunarity = 2.4f;
  const float persistance = .45f;
  const unsigned int seed = rand() % 100000;  // 52485;
  int octaves = sqrt(OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE);

  FastNoiseLite* noise;
  void initNoise();
  void generateTrees();
  void placeTreeAt(int x, int z, u8 treeHeight);
  u8 getBlock(int noise, int y);
  float getContinentalness(int x, int z);
  float getErosion(int x, int z);
  float getPeaksAndValleys(int x, int z);
  float getDensity(int x, int y, int z);
  float getTemperature(int x, int z);
  float getHumidity(int x, int z);
  float getHeightScale(int x, int z);

  void buildChunk(int offsetX, int offsetY, int offsetZ);
  u8 getBlockTypeByOffset(const int& x, const int& y, const int& z);
  unsigned int getIndexByOffset(int x, int y, int z);
  unsigned int getIndexByPosition(Vec4* pos);
  Vec4* getPositionByIndex(unsigned int index);
  bool isBlockHidden(int x, int y, int z);
  inline bool isBlockVisible(int x, int y, int z) {
    return !isBlockHidden(x, y, z);
  };

  void handlePadControls(Pad* t_pad);
  Vec4* normalizeWorldBlockPosition(Vec4* worldPosition);
};
