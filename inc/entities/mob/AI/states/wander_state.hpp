#pragma once

#include "entities/mob/AI/mob_state.hpp"
#include "entities/mob/AI/states/state_resolver.hpp"
#include "entities/mob/AI/a_star_path_finder.hpp"
#include "entities/entity.hpp"
#include "entities/level.hpp"
#include "entities/chunck.hpp"
#include "managers/chunck_manager.hpp"
#include <memory>

class StateResolver;

class WanderState : public MobState {
 public:
  int distance = CHUNCK_SIZE * 2;
  AStarPathFinder pathFinder;

  void update(Mob* pMob, const float& fixedDeltaTime) {
    if (pMob->currentPath) {
      if (pMob->advancePath(fixedDeltaTime)) {
        // Free the current path
        pMob->currentPath.reset();

        // Set new state to Idle
        StateResolver::SetState(pMob, MobStateType::Idle);
      }
    } else {
      Level* pLevel = Level::getInstance();
      Vec4 offsetStart = pLevel->worldPosToOffset(pMob->position);

      int randX = Tyra::Math::randomi(0, distance) - distance / 2;
      int randZ = Tyra::Math::randomi(0, distance) - distance / 2;
      Vec4 posOffset = Vec4(randX, 0, randZ);
      Vec4 offsetTarget = offsetStart + posOffset;

      // TODO: implement target height
      // offsetTarget.y = pChunkManager->getHeightAtOffset(offsetTarget);

      if (pLevel->BoundCheckMap(offsetTarget.x, offsetTarget.y,
                                offsetTarget.z)) {
        // Prevent to move to an unloaded chunk
        ChunckManager* pChunkManager = ChunckManager::getInstance();
        const Chunck* chk = pChunkManager->getChunkByBlockOffset(offsetTarget);
        if (chk && chk->state == ChunkState::Loaded) {
          pMob->currentPath.reset(new PathResult());
          if (!pathFinder.FindPath(offsetStart, offsetTarget,
                                   pMob->currentPath.get())) {
            pMob->currentPath.reset();
          }
        }
      }
    }
  };
};
