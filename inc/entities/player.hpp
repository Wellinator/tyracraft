/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#pragma once
#include "camera.hpp"
#include "contants.hpp"
#include "entities/Block.hpp"
#include "entities/item.hpp"
#include <tamtypes.h>
#include "managers/items_repository.hpp"
#include "managers/collision_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "loaders/3d/md2_loader/md2_loader.hpp"
#include "entities/items/materials.hpp"
#include "entities/items/tools/axe/axe.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::DynamicMesh;
using Tyra::DynamicPipeline;
using Tyra::DynPipOptions;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::MD2Loader;
using Tyra::MD2LoaderOptions;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::TextureRepository;
using Tyra::Timer;
using Tyra::Vec4;

/** Player 3D object class  */
class Player {
 public:
  std::unique_ptr<DynamicMesh> mesh;
  std::unique_ptr<DynamicMesh> armMesh;
  Player(Renderer* t_renderer, SoundManager* t_soundManager,
         BlockManager* t_blockManager);
  ~Player();

  void update(const float& deltaTime, Pad& t_pad, Camera& t_camera,
              std::vector<Block*> loadedBlocks);
  void render();

  inline Vec4* getPosition() { return mesh->getPosition(); };
  u8 isWalking, isOnGround, isBreaking;
  Vec4 spawnArea;
  u16 currentChunckId = 0;

  // Phisycs variables
  Ray ray;
  Block *currentBlock, *willCollideBlock = nullptr;

  // Inventory
  u8 inventoryHasChanged = 1;
  u8 selectedSlotHasChanged = 0;
  ItemId getSelectedInventoryItemType();
  u8 getSelectedInventorySlot();
  inline ItemId* getInventoryData() { return inventory; };
  inline BBox getHitBox() const {
    return hitBox->getTransformed(mesh->translation);
  };

 private:
  Renderer* t_renderer;
  BlockManager* t_blockManager;
  SoundManager* t_soundManager;
  StaticPipeline stpip;
  Vec4 getNextPosition(const float& deltaTime, Pad& t_pad,
                       const Camera& t_camera);
  u8 shouldRenderPlayerModel = 0;
  u8 isWalkingAnimationSet, isBreakingAnimationSet, isStandStillAnimationSet;
  u8 isOnBlock, isUnderBlock;
  Audio* t_audio;
  const u8 WALK_SFX_CH = 3;

  // Forces values
  float speed = 100;

  // Phisycs values
  Vec4 lift = Vec4(0.0f, -2.8F, 0.0f);
  Vec4 velocity = Vec4(0.0f, 0.0f, 0.0f);
  BBox* hitBox;

  void loadMesh();
  void loadArmMesh();
  void calcStaticBBox();
  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  void updateGravity(const float& deltaTime, const float terrainHeight);
  u8 updatePosition(Block** t_blocks, int blocks_ammount,
                    const float& deltaTime, const Vec4& nextPlayerPos,
                    u8 isColliding = 0);
  float getTerrainHeightOnPlayerPosition(Block** t_blocks, int blocks_ammount);
  void handleInputCommands(Pad& t_pad);

  // Inventory

  ItemId inventory[INVENTORY_SIZE] = {ItemId::empty,      ItemId::dirt,
                                      ItemId::stone,      ItemId::sand,
                                      ItemId::bricks,     ItemId::glass,
                                      ItemId::oak_planks, ItemId::spruce_planks,
                                      ItemId::stone_brick};  // Starts from 0

  short int selectedInventoryIndex = 0;
  void moveSelectorToTheLeft();
  void moveSelectorToTheRight();
  void playWalkSfx(u8 blockType);

  Axe* handledItem = new Axe(ItemsMaterials::Wood);

  DynPipOptions modelDynpipOptions;
  DynPipOptions armDynpipOptions;
  DynamicPipeline dynpip;

  // Animations
  // Player body
  float baseAnimationSpeed = 0.08F;
  std::vector<u32> walkSequence = {2, 1, 0, 1};
  std::vector<u32> breakBlockSequence = {9, 3, 4, 5, 6, 7, 8, 9};
  std::vector<u32> standStillSequence = {1};

  // Player arm
  std::vector<u32> armStandStillSequence = {6};
  std::vector<u32> armWalkingSequence = {0, 1, 2, 3,  4,  5, 6,
                                         7, 8, 9, 10, 11, 12};
  std::vector<u32> armHitingSequence = {13, 14, 15, 16, 17, 18, 19, 20};
};
