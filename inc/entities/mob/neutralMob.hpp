#pragma once

#include <tyra>
#include "constants.hpp"
#include "entities/mob/mob.hpp"

class NeutralMob : public Mob {
 public:
  NeutralMob(const MobType mobType);
  virtual ~NeutralMob();
  
  virtual void update(const float& deltaTime, const Vec4& movementDir,
                      LevelMap* t_terrain) = 0;
  virtual void render() = 0;
};
