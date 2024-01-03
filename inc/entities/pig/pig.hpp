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
#include "constants.hpp"
#include "entities/Block.hpp"
#include "entities/entity.hpp"
#include <tamtypes.h>
#include "managers/sound_manager.hpp"
#include "entities/items/materials.hpp"
#include "models/terrain_height_model.hpp"
#include "entities/chunck.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_config_model.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::DynamicMesh;
using Tyra::DynamicPipeline;
using Tyra::DynPipOptions;
using Tyra::FileUtils;
using Tyra::M4x4;
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
class Pig : public Entity {
 public:
  Pig(Renderer* t_renderer, SoundManager* t_soundManager);
  ~Pig();

  void update(const float& deltaTime, const Vec4& movementDir,
              LevelMap* t_terrain);
  void render();

  Vec4 spawnPosition;
  inline Vec4* getPosition() { return mesh->getPosition(); };
  bool isOnGround, isMoving;

  std::unique_ptr<DynamicMesh> mesh;

  // Phisycs variables
  Entity* underEntity = nullptr;
  Entity* overEntity = nullptr;

  inline BBox getHitBox() const {
    return bbox->getTransformed(mesh->translation);
  };

  DynPipOptions modelDynpipOptions;
  DynamicPipeline dynpip;

  Renderer* t_renderer;

  void setWalkingAnimation();
  void unsetWalkingAnimation();
  void jump();
  void swim();

  bool isOnWater();

 private:
  Vec4 getNextPosition(const float& deltaTime,const Vec4& direction);

  SoundManager* t_soundManager;
  Audio* t_audio;

  bool isWalkingAnimationSet, isStandStillAnimationSet;

  // Forces values
  float acceleration = 100.0F;
  float speed = 0;
  float maxSpeed = 20.0F;

  void updateTerrainHeightAtEntityPosition(const Vec4 nextVrticalPosition);
  TerrainHeightModel terrainHeight;

  // Phisycs values
  Vec4 lift = Vec4(0.0f, 125.0F, 0.0f);
  Texture* texture;

  void loadTexture();
  void loadMesh();
  void loadStaticBBox();
  void getMinMax(const Mesh& t_mesh, Vec4& t_min, Vec4& t_max);
  Vec4 getNextVrticalPosition(const float& deltaTime);
  void updateGravity(const Vec4 nextVerticalPosition);
  u8 updatePosition(const float& deltaTime, const Vec4& nextPosition,
                    u8 isColliding = 0);

  float lastTimePlayedSfx = 0.0F;
  void playSfx(const Blocks& blockType);

  void animate();

  // Animations
  float baseAnimationSpeed = 0.1F;
  std::vector<u32> standStillSequence = {0};
  std::vector<u32> walkSequence = {1, 2, 1};

  u8 _isOnWater;
  void updateStateInWater(LevelMap* terrain);
};
