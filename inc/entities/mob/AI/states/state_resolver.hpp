#pragma once

#include "entities/mob/mob.hpp"

typedef enum { Wander, Idle } MobStateType;

class StateResolver {
 public:
  static void SetState(Mob* pMob, MobStateType targetStateType);
};
