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
#include "entities/entity.hpp"
#include "entities/animation/animated.hpp"
#include <tamtypes.h>
#include "managers/items_repository.hpp"
#include "managers/sound_manager.hpp"
#include "managers/items_repository.hpp"
#include "loaders/3d/md2_loader/md2_loader.hpp"
#include "entities/items/materials.hpp"
#include "entities/items/tools/axe/axe.hpp"
#include "entities/chunk.hpp"
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player_render_arm_pip.hpp"
#include "entities/player/player_render_body_pip.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_config_model.hpp"
#include "models/world_light_model.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::MD2Loader;
using Tyra::MD2LoaderOptions;
using Tyra::MeshBuilderData;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::PadButtons;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Texture;
using Tyra::TextureRepository;
using Tyra::Timer;
using Tyra::Vec4;

/** Player 3D object class  */

// TODO: showld inherit Mob class instead Entity directly
class Player : public Entity, public Animated {
 public:
  Player(Level* pLevel, Renderer* t_renderer, ItemRepository* t_itemRepository,
         WorldLightModel* t_worldLightModel);
  ~Player();

  void fixedUpdate(const float& fixedDeltaTime, const Vec4& movementDir,
                   Camera* t_camera);
  void update(const float& deltaTime, Camera* t_camera);
  void tick() override;
  void render();

  void setRenderArmPip();
  void setRenderBodyPip();

  void toggleFlying();
  inline void unFly() {
    _isFlying = false;
    isOnGround = false;
  };

  Level* pLevel;

  inline Vec4* getPosition() { return &position; };

  void setPosition(const Vec4& pos) {
    position.set(pos);
    _prevPosition.set(position);
    _targetPosition.set(position);
  };

  inline const u8 isHandFree() { return !isHoldingAnItem(); };
  inline const u8 isHoldingAnItem() {
    return getSelectedInventoryItemType() != ItemId::empty;
  };

  void playPutBlockAnimation();
  void stopPutBlockAnimation();

  // Frame [1] - Idle
  // Frame [2, 3] - Walk
  std::array<std::unique_ptr<Tyra::Mesh>, 10> playerFrames;

  Vec4 spawnArea;
  u16 currentChunkId = 0;

  // Phisycs variables
  Ray ray;

  // Override Entity
  const float getHeight() override;
  void resolveOutOfWorldBoundaries() override;

  // Inventory
  u8 inventoryHasChanged = 1;
  u8 selectedSlotHasChanged = 0;
  ItemId getSelectedInventoryItemType();
  u8 getSelectedInventorySlot();
  void fillInventoryWithItem(ItemId);
  inline void updateHandledItem() {
    this->renderPip->unloadItemDrawData();
    this->renderPip->loadItemDrawData();
  };
  inline ItemId* getInventoryData() { return inventory; };

  const Vec4 getHitBoxSize() override {
    return Vec4((DOUBLE_BLOCK_SIZE * 0.3F) / 2, DOUBLE_BLOCK_SIZE * 1.8F,
                (DOUBLE_BLOCK_SIZE * 0.3F) / 2);
  };

  Renderer* t_renderer;
  std::unique_ptr<PlayerRenderPip> renderPip;

  void moveSelectorToTheLeft();
  void moveSelectorToTheRight();

  void selectNextItem();
  void selectPreviousItem();
  void jump();
  void jumpQuickly();
  void swim();
  void flyUp(const float& deltaTime);
  void flyDown(const float& deltaTime);
  void shiftItemToInventory(const ItemId& itemToShift);
  void setItemToInventory(const ItemId& itemToShift);
  void setRunning(bool _isRunning);
  void setWalkingAnimation();
  void setIdleAnimation();
  void setArmBreakingAnimation();
  void setArmIdleAnimation();
  bool isPuttingBlock() { return _isPuting; };
  bool isBreakingBlock() { return _isBreaking; };
  bool isFlying() { return _isFlying; };
  bool isRunning() { return _isRunning; };

  void fillAnimationDrawData(std::vector<Vec4>* pVertices,
                             std::vector<Color>* pVerticesColors,
                             std::vector<Vec4>* pUvMap);

  bool isOnWater();
  bool isUnderWater();
  void updateStateInWater();

  inline Texture* getPlayerTexture() { return playerTexture; };

  ItemRepository* t_itemRepository;
  WorldLightModel* t_worldLightModel;

  inline Color* getBaseColorAtPlayerPos() { return &_baseColorAtPlayerPos; };

  // TickScheduler integration
  void registerTickCallbacks(class TickScheduler& scheduler);

 private:
  class TickTaskHandles* tickHandles;
  Vec4 getNextXZPosition(const float& deltaTime, const Vec4& sensibility,
                         const Vec4& camDir);
  Audio* t_audio;

  // State control
  bool _isFlying, _isBreaking, _isPuting, _isRunning;

  // Forces values
  float acceleration = 140.0F;
  float speed = 0;
  float maxSpeed = 60.0F;

  float runningAcceleration = 170.0F;
  float runningMaxSpeed = 100.0F;

  // Phisycs values
  Vec4 lift = Vec4(0.0f, 125.0F, 0.0f);
  Texture* playerTexture;

  void loadPlayerTexture();
  void loadMesh();
  void loadStaticBBox();
  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  float getNextVrticalPosition(const float& deltaTime);
  void fly(const float& deltaTime, const TerrainHeightModel& terrainHeight,
           const Vec4& direction);
  u8 updateXZPosition(const float& deltaTime, const Vec4& nextPlayerPos,
                      u8 isColliding = 0);

  // Inventory

  ItemId inventory[HOT_INVENTORY_SIZE] = {
      ItemId::empty, ItemId::empty, ItemId::empty, ItemId::empty, ItemId::empty,
      ItemId::empty, ItemId::empty, ItemId::empty, ItemId::empty,
  };
  short int selectedInventoryIndex = 0;

  float lastTimePlayedWalkSfx = 0.0F;
  void onMoved();
  void onStopMoving();
  void playWalkSfx(const Blocks& blockType);
  void playSwimSfx();
  void playSplashSfx();
  u8 isSubmerged = false;

  // Axe* handledItem = new Axe(ItemsMaterials::Wood);

  // Animations
  // Player body
  const u8 IDLE_ANIMATION = 0;
  const u8 WALK_ANIMATION = 1;
  const u8 BREAKING_ANIMATION = 2;
  void loadAnimations();

  const float _minFov = 60.0F;
  const float _maxFov = _minFov + 10.0F;
  const float _minFovFlaying = 70.0F;
  const float _maxFovFlaying = _minFovFlaying + 10.0F;
  void updateFovBySpeed();

  void updateItemColorByCurrentPosition();
  Color _baseColorAtPlayerPos = Color(128, 128, 128);
};
