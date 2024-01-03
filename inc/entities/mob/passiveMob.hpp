#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/mob/mob.hpp"

class PassiveMob : public Mob {
 public:
  PassiveMob(const MobType mobType);
  virtual ~PassiveMob();

  virtual void update(const float& deltaTime, const Vec4& movementDir,
                      LevelMap* t_terrain) = 0;
  virtual void render() = 0;
};
