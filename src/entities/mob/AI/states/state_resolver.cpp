
#include "entities/mob/AI/states/state_resolver.hpp"
#include "entities/mob/AI/states/wander_state.hpp"
#include "entities/mob/AI/states/idle_state.hpp"
#include <memory>

void StateResolver::SetState(Mob* pMob, MobStateType targetStateType) {
  if (pMob->currentState != nullptr) delete pMob->currentState;

  switch (targetStateType) {
    case MobStateType::Wander:
      pMob->currentState = new WanderState();
      break;

    default:
      pMob->currentState =
          new IdleState(MobStateType::Wander, Tyra::Math::randomi(1, 5));

      break;
  }
}