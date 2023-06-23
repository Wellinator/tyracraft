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
#include "constants.hpp"
#include "entities/Block.hpp"
#include "entities/item.hpp"
#include <tamtypes.h>
#include "managers/items_repository.hpp"
#include "managers/block_manager.hpp"
#include "managers/sound_manager.hpp"
#include "loaders/3d/md2_loader/md2_loader.hpp"
#include "entities/items/materials.hpp"
#include "entities/items/tools/axe/axe.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/chunck.hpp"
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player_first_person_render_pip.hpp"
#include "entities/player/player_third_person_render_pip.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_config_model.hpp"
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
using Tyra::PadButtons;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::TextureRepository;
using Tyra::Timer;
using Tyra::Vec4;

/** Player 3D object class  */
class Player {
 public:
  Player(Renderer* t_renderer, SoundManager* t_soundManager,
         BlockManager* t_blockManager);
  ~Player();

  void update(const float& deltaTime, const Vec4& movementDir, Camera* t_camera,
              const std::vector<Chunck*>& loadedChunks,
              TerrainHeightModel* terrainHeight, LevelMap* t_terrain);
  void render();

  void toogleCameraMode();
  u8 isFirstPerson = true;

  void toggleFlying();
  inline Vec4* getPosition() { return mesh->getPosition(); };
  bool isOnGround, isFlying, isBreaking, isMoving, isRunning;

  inline const u8 isHandFree() { return !isHoldingAnItem(); };
  inline const u8 isHoldingAnItem() {
    return getSelectedInventoryItemType() != ItemId::empty;
  };

  std::unique_ptr<DynamicMesh> mesh;
  std::unique_ptr<DynamicMesh> armMesh;

  Vec4 spawnArea;
  u16 currentChunckId = 0;

  // Phisycs variables
  Ray ray;
  Block* currentBottomBlock = nullptr;
  Block* currentUpperBlock = nullptr;

  // Inventory
  u8 inventoryHasChanged = 1;
  u8 selectedSlotHasChanged = 0;
  ItemId getSelectedInventoryItemType();
  u8 getSelectedInventorySlot();
  inline ItemId* getInventoryData() { return inventory; };
  inline BBox getHitBox() const {
    return hitBox->getTransformed(mesh->translation);
  };

  DynPipOptions modelDynpipOptions;
  DynPipOptions armDynpipOptions;
  DynamicPipeline dynpip;

  Renderer* t_renderer;
  PlayerRenderPip* renderPip = nullptr;

  void moveSelectorToTheLeft();
  void moveSelectorToTheRight();
  void setArmBreakingAnimation();
  void unsetArmBreakingAnimation();
  void setWalkingAnimation();
  void unsetWalkingAnimation();
  void selectNextItem();
  void selectPreviousItem();
  void jump();
  void swim();
  void flyUp(const float& deltaTime, const TerrainHeightModel& terrainHeight);
  void flyDown(const float& deltaTime, const TerrainHeightModel& terrainHeight);
  void shiftItemToInventory(const ItemId& itemToShift);
  void setItemToInventory(const ItemId& itemToShift);
  void setRunning(bool _isRunning);

  TerrainHeightModel getTerrainHeightAtPosition(
      const std::vector<Chunck*>& loadedChunks);

  bool isOnWater();
  bool isUnderWater();

 private:
  BlockManager* t_blockManager;
  SoundManager* t_soundManager;
  StaticPipeline stpip;
  Vec4 getNextPosition(const float& deltaTime, const Vec4& sensibility,
                       const Vec4& camDir);
  bool isWalkingAnimationSet, isBreakingAnimationSet, isStandStillAnimationSet;
  Audio* t_audio;

  void setRenderPip(PlayerRenderPip* pipToSet);

  // Forces values
  float acceleration = 140.0F;
  float speed = 0;
  float maxSpeed = 60.0F;

  float runningAcceleration = 170.0F;
  float runningMaxSpeed = 100.0F;

  // Phisycs values
  Vec4 lift = Vec4(0.0f, 125.0F, 0.0f);
  Vec4 velocity = Vec4(0.0f);
  BBox* hitBox;
  Texture* playerTexture;

  void loadPlayerTexture();
  void loadMesh();
  void loadArmMesh();
  void calcStaticBBox();
  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  void updateGravity(const float& deltaTime, TerrainHeightModel* terrainHeight);
  void fly(const float& deltaTime, const TerrainHeightModel& terrainHeight,
           const Vec4& direction);
  u8 updatePosition(const std::vector<Chunck*>& loadedChunks,
                    const float& deltaTime, const Vec4& nextPlayerPos,
                    u8 isColliding = 0);

  // Inventory

  ItemId inventory[HOT_INVENTORY_SIZE] = {
      ItemId::empty, ItemId::empty, ItemId::empty, ItemId::empty, ItemId::empty,
      ItemId::empty, ItemId::empty, ItemId::empty, ItemId::empty,
  };

  short int selectedInventoryIndex = 0;

  float lastTimePlayedWalkSfx = 0.0F;
  void playWalkSfx(const Blocks& blockType);
  void playSwimSfx();
  void playSplashSfx();
  u8 isSubmerged = false;

  void animate();

  Axe* handledItem = new Axe(ItemsMaterials::Wood);

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

  u8 _isOnWater;
  u8 _isUnderWater;
  void updateStateInWater(LevelMap* terrain);
};
