#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/entity.hpp"

class Mob : public Entity {
 public:
  Mob(const MobType mobType) : Entity(EntityType::Mob), type(mobType){};

  virtual ~Mob(){};

  /** Mob type */
  const MobType type;

  /** Mod id */
  const uint32_t id = std::rand() % 999999;
};
