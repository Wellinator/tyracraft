#pragma once

#include "entities/mob/AI/mob_state.hpp"
#include "entities/mob/AI/states/state_resolver.hpp"
#include "entities/mob/AI/a_star_path_finder.hpp"
#include "entities/entity.hpp"
#include "managers/chunk_manager.hpp"

class StateResolver;

class IdleState : public MobState {
 public:
  float expiry;
  MobStateType nextState = MobStateType::Wander;

  IdleState(MobStateType nextStateType, float timeToExpiry = 0) {
    expiry = timeToExpiry;
    nextState = nextStateType;
  };

  void update(Mob* pMob, const float& fixedDeltaTime) {
    expiryCounter += fixedDeltaTime;
    if (expiryCounter >= expiry) {
      StateResolver::SetState(pMob, nextState);
    }
  };

 private:
  float expiryCounter = 0;
};
