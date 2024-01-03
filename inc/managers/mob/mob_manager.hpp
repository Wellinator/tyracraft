#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>
#include "models/world_light_model.hpp"
#include "entities/mob/mob.hpp"
#include "entities/level.hpp"
#include "managers/sound_manager.hpp"
#include "3libs/FastNoiseLite/ModdedFastNoiseLite.h"

using Tyra::Color;
using Tyra::DynamicPipeline;
using Tyra::DynPipOptions;
using Tyra::Renderer;
using Tyra::Texture;
using Tyra::Vec4;

class MobManager {
 public:
  MobManager();
  ~MobManager();

  void init(Renderer* renderer, SoundManager* t_soundManager,
            WorldLightModel* t_worldLightModel, LevelMap* t_terrain);
  void update(const float& deltaTime);
  void render();

  Mob* spawnMob(const MobType type);
  Mob* spawnMobAtPosition(const MobType type, const Vec4& position);
  void unspawnMob(const uint32_t id);

 private:
  // TODO: move to mob AI
  float changeDirectionTimer = 1;
  float changeDirectionLimit = 0;

  Renderer* t_renderer = nullptr;
  SoundManager* t_soundManager = nullptr;
  WorldLightModel* t_worldLightModel = nullptr;
  LevelMap* t_terrain = nullptr;

  // Mobs textures
  Texture* pigTexture = nullptr;

  std::vector<Mob*> mobs;

  DynPipOptions dynpipOptions;
  DynamicPipeline dynpip;

  void _loadPigTexture();

  Vec4 _getMobMoviementDirection(Mob* mob);

  Mob* _createPig();
  Mob* _createPigAtPosition(const Vec4& position);
};
