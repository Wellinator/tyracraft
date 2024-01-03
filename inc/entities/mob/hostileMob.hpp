#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/mob/mob.hpp"

class HostileMob : public Mob {
 public:
  HostileMob(const MobType mobType);
  virtual ~HostileMob();

  virtual void update(const float& deltaTime, const Vec4& movementDir,
                      LevelMap* t_terrain) = 0;
  virtual void render() = 0;
};
