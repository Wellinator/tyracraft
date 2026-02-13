#pragma once

#include "entities/mob/AI/mob_state.hpp"
#include "entities/mob/AI/states/state_resolver.hpp"
#include "entities/mob/AI/a_star_path_finder.hpp"
#include "entities/entity.hpp"
#include "entities/level.hpp"
#include "entities/chunk.hpp"
#include "managers/chunk_manager.hpp"
#include "debug.hpp"
#include <memory>

class StateResolver;

class WanderState : public MobState {
 public:
  int distance = CHUNK_SIZE;
  AStarPathFinder pathFinder;

  void update(Mob* pMob, const float& fixedDeltaTime) {
    timerCounter += fixedDeltaTime;

    if (pMob->currentPath) {
      if (pMob->advancePath(fixedDeltaTime) || timerCounter >= LIMIT_TO_GOAL) {
        timerCounter = 0.0f;

        // Free the current path
        pMob->currentPath.reset();

        // Set new state to Idle
        StateResolver::SetState(pMob, MobStateType::Idle);
      }
    } else {
      // Add cooldown to avoid running A* pathfinding every frame
      pathfindCooldown -= fixedDeltaTime;
      if (pathfindCooldown > 0.0f) return;
      pathfindCooldown = PATHFIND_COOLDOWN_TIME;

      Level* pLevel = Level::getInstance();
      Vec4 offsetStart = pLevel->worldPosToOffset(pMob->position);

      int randX = Tyra::Math::randomi(0, distance) - distance / 2;
      int randZ = Tyra::Math::randomi(0, distance) - distance / 2;
      Vec4 posOffset = Vec4(randX, 0, randZ);
      Vec4 offsetTarget = offsetStart + posOffset;

      ChunkManager* pChunkManager = ChunkManager::getInstance();
      offsetTarget.y = pChunkManager->getHeightAtOffset(offsetTarget);

      if (pLevel->BoundCheckMap(offsetTarget.x, offsetTarget.y,
                                offsetTarget.z)) {
#ifdef DEBUG_MODE
        if (g_debug_menu.logPathfinding) {
          offsetStart.print("From: ");
          offsetTarget.print("To: ");
        }
#endif

        // Prevent to move to an unloaded chunk
        const Chunk* chk = pChunkManager->getChunkByBlockOffset(offsetTarget);
        if (chk && chk->isLoaded()) {
          pMob->currentPath.reset(new PathResult());
          if (!pathFinder.FindPath(offsetStart, offsetTarget,
                                   pMob->currentPath.get())) {
            pMob->currentPath.reset();
          }
        }
      }
    }
  };

 private:
  //  Limit in seconds to reach the goal
  float timerCounter = 0.0f;
  const float LIMIT_TO_GOAL = 15.0f;

  // Cooldown between pathfinding attempts to avoid CPU spikes
  float pathfindCooldown = 0.0f;
  const float PATHFIND_COOLDOWN_TIME = 0.5f;
};
