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
#include "entities/mob/passiveMob.hpp"
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
class Pig : public PassiveMob {
 public:
  Pig(Renderer* t_renderer, SoundManager* t_soundManager,
      ChunckManager* t_chunkManager, Texture* pigTexture,
      DynamicMesh* baseMesh);
  ~Pig();

  void update(const float& deltaTime, const Vec4& movementDir,
              LevelMap* t_terrain);
  void render(){};

  inline Vec4* getPosition() { return mesh->getPosition(); };
  bool isOnGround, isMoving;

  Chunck* currentChunck = nullptr;

  // Phisycs variables
  Entity* underEntity = nullptr;
  Entity* overEntity = nullptr;

  BBox getHitBox() const {
    const auto rawAABB = mesh->getCurrentBoundingBox();
    const auto Obb = rawAABB.getTransformed(mesh->getModelMatrix());
    const auto aabb = BBox(Obb.vertices, Obb.getVertexCount());

    return aabb;
  };

  Renderer* t_renderer;

  void setWalkingAnimation();
  void updateWalkingAnimationSpeed();
  void unsetWalkingAnimation();
  void jump();
  void swim();

  bool isOnWater();

 private:
  Vec4 getNextPosition(const float& deltaTime, const Vec4& direction);

  SoundManager* t_soundManager;
  ChunckManager* t_chunkManager;
  Audio* t_audio;

  bool isWalkingAnimationSet, isStandStillAnimationSet;

  // Forces values
  float acceleration = 80.0F;
  float speed = 0;
  float maxSpeed = 20.0F;

  void updateTerrainHeightAtEntityPosition(const Vec4 nextVrticalPosition,
                                           Vec4* minEntityPos,
                                           Vec4* maxEntityPos);
  TerrainHeightModel terrainHeight;

  // Phisycs values
  Vec4 lift = Vec4(0.0f, 125.0F, 0.0f);
  Texture* texture;

  void loadMesh(DynamicMesh* baseMesh);
  void loadStaticBBox();
  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  Vec4 getNextVrticalPosition(const float& deltaTime);
  void updateGravity(const Vec4 nextVerticalPosition, BBox* bbox,
                     Vec4* entityMin, Vec4* entityMax);
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
  inline const u8 canPlayStepSfx() {
    return lastTimePlayedStepSfx > stepSfxLimit;
  }
  inline const u8 canPlaySaySfx() { return lastTimePlayedSaySfx > saySfxLimit; }

  // Animations
  float baseAnimationSpeed = 0.35F;
  std::vector<u32> standStillSequence = {0};
  std::vector<u32> walkSequence = {1, 2};

  u8 _isOnWater;
  void updateStateInWater(LevelMap* terrain, Vec4* min, Vec4* max);
};
