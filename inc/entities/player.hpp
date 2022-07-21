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
#include <pad/pad.hpp>
#include <math/vec4.hpp>
#include <time/timer.hpp>
#include <physics/ray.hpp>
#include <tamtypes.h>
#include <audio/audio.hpp>
#include "managers/items_repository.hpp"
#include <renderer/renderer.hpp>
#include "managers/collision_manager.hpp"

using Tyra::Audio;
using Tyra::Mesh;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::TextureRepository;
using Tyra::Timer;
using Tyra::Vec4;

/** Player 3D object class  */
class Player {
 public:
  Mesh* mesh;
  Player(Renderer* t_renderer, Audio* t_audio);
  ~Player();

  void update(const float& deltaTime, Pad& t_pad, Camera& t_camera,
              Block* t_blocks[], unsigned int blocks_ammount);
  inline Vec4* getPosition() { return mesh->getPosition(); };
  u8 isFighting, isWalking, isOnGround;
  Vec4 spawnArea;

  // Phisycs variables
  Ray ray;
  Vec4* nextPlayerPos;
  Block *currentBlock, *willCollideBlock;
  Vec4 willCollideBlockMin, willCollideBlockMax, hitPosition;
  float distanceToHit = -1.0f;

  // Inventory
  u8 inventoryHasChanged = 1;
  u8 selectedSlotHasChanged = 0;
  ITEM_TYPES getSelectedInventoryItemType();
  u8 getSelectedInventorySlot();
  ITEM_TYPES* getInventoryData() { return inventory; };

 private:
  Renderer* t_renderer;
  Vec4* getNextPosition(const float& deltaTime, Pad& t_pad,
                        const Camera& t_camera);
  u8 isWalkingAnimationSet, isJumpingAnimationSet, isFightingAnimationSet;
  u8 isOnBlock, isUnderBlock;
  Audio* t_audio;
  Timer walkTimer, fightTimer;
  audsrv_adpcm_t *walkAdpcm, *jumpAdpcm, *boomAdpcm;

  // Forces values
  float speed = 100;
  Vec4 velocity, lift;

  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  void updatePosition(const float& deltaTime);
  void updateGravity(const float& deltaTime);
  void checkIfWillCollideBlock(Block* t_blocks[], int blocks_ammount);
  void checkIfIsOnBlock(Block* t_blocks[], int blocks_ammount);
  void handleInputCommands(Pad& t_pad);

  // Inventory

  ITEM_TYPES inventory[INVENTORY_SIZE] = {
      ITEM_TYPES::dirt,
      ITEM_TYPES::stone,
      ITEM_TYPES::sand,
      ITEM_TYPES::bricks,
      ITEM_TYPES::glass,
      ITEM_TYPES::oak_planks,
      ITEM_TYPES::spruce_planks,
      ITEM_TYPES::stone_brick,
      ITEM_TYPES::chiseled_stone_bricks};  // Starts from 0
  short int selectedInventoryIndex = 0;
  void moveSelectorToTheLeft();
  void moveSelectorToTheRight();
};
