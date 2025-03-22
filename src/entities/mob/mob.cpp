#include "entities/mob/mob.hpp"
#include "entities/mob/AI/states/wander_state.hpp"
#include "utils.hpp"

Mob::Mob(Level* level) : Entity(level, EntityType::Mob) {
  currentState = new WanderState();
};

Mob::~Mob(){};

void Mob::fixedUpdate(const float& fixedDeltaTime) {
  if (currentState != nullptr) {
    currentState->update(this, fixedDeltaTime);
  }
}

void Mob::update(const float& deltaTime) {}

void Mob::jump() {
  velocity += lift;
  isOnGround = false;
}

bool Mob::advancePath(const float& fixedDeltaTime, bool faceRoute) {
  onMoved();

  Vec4 target = currentPath->waypoints[currentPath->currentIndex];

  if (faceRoute) {
    // Update mesh rotation; This routine do not change the hitbox
    Vec4 dir = (target - position).getNormalized();
    mesh.get()->rotation.identity();
    float revTheta = Utils::reverseAngle(Tyra::Math::atan2(dir.x, dir.z));
    mesh.get()->rotation.rotateY(revTheta);
  };

  // TODO: lookAt

  // Try to move horizontally;
  Vec4 nextXZPos = getNextXZPosition(fixedDeltaTime, target);
  const bool hasMovedHorizontally = updateXZPosition(fixedDeltaTime, nextXZPos);
  if (hasMovedHorizontally) {
    if (position.distanceTo(target) <= 0.15f) {
      currentPath->currentIndex++;

      // If it was the the final step, return true.
      if (currentPath->currentIndex >= currentPath->waypoints.size()) {
        onStopMoving();
        return true;
      }
    }
  } else {
    // If couldn't move horizontaly and target is higher than current position,
    // try to jump;
    if (target.y > position.y && isOnGround) jump();
  }

  return false;
}