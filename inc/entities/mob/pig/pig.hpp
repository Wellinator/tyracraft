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

#include <tyra>
#include <memory>
#include "constants.hpp"
#include "entities/Block.hpp"
#include "entities/chunck.hpp"
#include "entities/mob/mob.hpp"
#include <tamtypes.h>
#include <array>
#include "managers/chunck_manager.hpp"
#include "managers/sound_manager.hpp"
#include "entities/items/materials.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/chunck.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_config_model.hpp"
#include "entities/level.hpp"

using Tyra::Audio;
using Tyra::BBox;
using Tyra::CoreBBox;
using Tyra::DynamicMesh;
using Tyra::DynamicPipeline;
using Tyra::DynPipOptions;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::MeshBuilderData;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::PadButtons;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::TextureRepository;
using Tyra::Timer;
using Tyra::Vec4;

/** Pig 3D object class  */
class Pig : public Mob {
 public:
  Pig(Level* level, Renderer* t_renderer, ChunckManager* t_chunkManager,
      Texture* pigTexture, DynamicMesh* baseMesh);
  ~Pig();

  // Override Mob
  void fixedUpdate(const float& fixedDeltaTime, const Vec4& movementDir);
  void update(const float& deltaTime);
  void render(){};

  Renderer* t_renderer;
  Chunck* currentChunck = nullptr;
  Level* pLevel;

  void setWalkingAnimation();
  void updateWalkingAnimationSpeed();
  void unsetWalkingAnimation();
  void jump();
  void jumpQuickly();
  void swim();
  bool isOnWater();
  bool isUnderWater();

  // Override Mob
  /** Mob category */
  virtual MobCategory getCategory() override;

  /** Mob type */
  virtual MobType getType() override;

  // Override Entity
  void tick() override;
  const float getHeight() override;
  void resolveOutOfWorldBoundaries() override;

  void onMoved();
  void onStopMoving();

 private:
  Vec4 getNextXZPosition(const float& deltaTime, const Vec4& movementDir);
  u8 updateXZPosition(const float& deltaTime, const Vec4& nextPosition,
                      u8 isColliding = 0);

  ChunckManager* t_chunkManager;
  Audio* t_audio;

  bool isSubmerged = false;
  bool isWalkingAnimationSet, isStandStillAnimationSet;

  // Forces values
  const float acceleration = 80.0F;
  const float maxSpeed = 20.0F;
  float speed = 0.0F;

  const Vec4 hitBoxDimensions =
      Vec4((DUBLE_BLOCK_SIZE * 0.9F) / 2, DUBLE_BLOCK_SIZE * 0.9F,
           (DUBLE_BLOCK_SIZE * 0.9F) / 2);

  // Phisycs values
  Vec4 lift = Vec4(0.0f, 125.0F, 0.0f);
  Texture* texture;

  void loadMesh(DynamicMesh* baseMesh);
  void loadStaticBBox();
  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  float getNextVrticalPosition(const float& deltaTime);
  u8 updatePosition(const float& deltaTime, const Vec4& nextPosition,
                    BBox* entityBB, Vec4* entityMin, Vec4* entityMax,
                    u8 isColliding = 0);

  // Sound control timers;
  float stepSfxLimit = 0.25f;
  float saySfxLimit = 0.70f;
  float lastTimePlayedSaySfx = 0;
  float lastTimePlayedStepSfx = 0;
  void playStepSfx();
  void playSaySfx();
  void playDeathSfx();
  void playSwimSfx();
  void playSplashSfx();

  inline const u8 canPlaySaySfx() { return lastTimePlayedSaySfx > saySfxLimit; }
  inline const u8 canPlayStepSfx() {
    return isOnGround && underEntity &&
           underEntity->entity_type == EntityType::Block;
  }

  // Animations
  const float ANIMATION_SPEED_FACT = 0.2f;
  std::vector<u32> standStillSequence = {0};
  std::vector<u32> walkSequence = {1, 2};

  void updateStateInWater();
};
