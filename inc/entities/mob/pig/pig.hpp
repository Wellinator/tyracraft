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
#include "entities/chunk.hpp"
#include "entities/mob/mob.hpp"
#include "entities/animation/animated.hpp"
#include <tamtypes.h>
#include <array>
#include "managers/chunk_manager.hpp"
#include "managers/sound_manager.hpp"
#include "entities/items/materials.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/chunk.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_config_model.hpp"
#include "entities/level.hpp"

using Tyra::Audio;
using Tyra::BBox;
using Tyra::CoreBBox;

using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::MeshBuilderData;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::PadButtons;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::StaPipOptions;
using Tyra::StaticPipeline;
using Tyra::TextureRepository;
using Tyra::Timer;
using Tyra::Vec4;

/** Pig 3D object class  */
class Pig : public Mob, public Animated {
 public:
  Pig(Level* level, Renderer* pRenderer, Texture* pigTexture,
      Tyra::Mesh** framesArray, const int size);
  ~Pig();

  // Override Mob
  void fixedUpdate(const float& fixedDeltaTime);
  void update(const float& deltaTime);
  void render();

  Renderer* t_renderer;
  Level* pLevel;

  void setWalkingAnimation();
  void setIdleAnimation();
  void updateWalkingAnimationSpeed();
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

  Vec4 getNextXZPosition(const float& fixedDeltaTime, const Vec4& target);
  bool updateXZPosition(const float& fixedDeltaTime, const Vec4& nextPosition,
                        u8 isColliding = 0);

  ChunkManager* t_chunkManager;
  Audio* t_audio;

  bool isSubmerged = false;
  bool isWalkingAnimationSet, isStandStillAnimationSet;

  // Forces values
  const float acceleration = 80.0F;
  const float maxSpeed = 20.0F;
  float speed = 25.0F;

  const Vec4 hitBoxDimensions =
      Vec4((DOUBLE_BLOCK_SIZE * 0.9F) / 2, DOUBLE_BLOCK_SIZE * 0.9F,
           (DOUBLE_BLOCK_SIZE * 0.9F) / 2);

  Texture* texture;

  void loadStaticBBox();
  float getNextVrticalPosition(const float& fixedDeltaTime);

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

  // Animations
  const float ANIMATION_SPEED_FACT = 0.2f;

  void updateStateInWater();

 private:
  StaPipOptions statPipOptions;
  StaticPipeline statPip;

  const u8 IDLE_ANIMATION = 0;
  const u8 WALK_ANIMATION = 1;
};
