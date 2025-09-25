#pragma once

#include "constants.hpp"
#include <tyra>
#include <memory>
#include <math.h>
#include "models/world_light_model.hpp"
#include "entities/mob/mob.hpp"
#include "entities/level.hpp"
#include "managers/chunk_manager.hpp"
#include "managers/sound_manager.hpp"
#include "3libs/FastNoiseLite/ModdedFastNoiseLite.h"

using Tyra::Color;
using Tyra::DynamicMesh;
using Tyra::MeshBuilderData;
using Tyra::Renderer;

using Tyra::Texture;
using Tyra::Vec4;

class MobManager {
 public:
  MobManager();
  ~MobManager();

  void init(Renderer* renderer, WorldLightModel* t_worldLightModel,
            Level* level, ChunkManager* t_chunkManager);
  void fixedUpdate(const float& fixedDeltaTime);
  void update(const float& deltaTime);
  void tick();
  void render();

  Mob* spawnMob(const MobType type);
  Mob* spawnMobAtPosition(const MobType type, const Vec4& position);
  void unspawnMob(const uint32_t id);

  // TODO: should be test to check PS2 limits
  static const u8 MAX_MOBS_LIMIT = 10;

 private:
  Renderer* t_renderer = nullptr;
  ChunkManager* t_chunkManager = nullptr;
  WorldLightModel* t_worldLightModel = nullptr;
  Level* pLevel = nullptr;

  // Mobs textures
  Texture* pigTexture = nullptr;
  Texture* cowTexture = nullptr;

  /**
   * Mobs mesh
   */
  // Frame [1] - Idle
  // Frame [2, 3] - Walk
  std::array<std::unique_ptr<Tyra::Mesh>, 3> pigFrames;

  // Frame [1] - Idle
  // Frame [2, 3] - Walk
  std::array<std::unique_ptr<Tyra::Mesh>, 3> cowFrames;

  u8 _mobsHasChanged = false;
  std::vector<Mob*> mobs;

  void _loadMobTextures();
  void _loadMobFrames();

  Vec4 _getMobMoviementDirection(Mob* mob);

  Mob* _createMob(const MobType type);
  Mob* _createMobAtPosition(const MobType type, const Vec4& position);

  void _destroyUnspownedMobs();
};
