#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/entity.hpp"
#include "entities/level.hpp"

using Tyra::DynamicMesh;

class Mob : public Entity {
 public:
  Mob(const MobCategory mobCategory, const MobType mobType)
      : Entity(EntityType::Mob), category(mobCategory), type(mobType){};

  virtual ~Mob(){};

  virtual void update(const float& deltaTime, const Vec4& movementDir,
                      LevelMap* t_terrain) = 0;
  virtual void render() = 0;

  /** Mob type */
  const MobCategory category;
  const MobType type;

  /** Mod id */
  const uint32_t id = std::rand() % 999999;

  /** Mob mesh data */
  std::unique_ptr<DynamicMesh> mesh;

  Vec4 spawnPosition;

  // TODO: move to mob AI
  /** Temp last moviment dir */
  Vec4 moviemntDirection;
};
