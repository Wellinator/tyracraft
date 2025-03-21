#include "entities/mob/mob.hpp"
#include "entities/mob/AI/states/wander_state.hpp"
#include "utils.hpp"

Mob::Mob(Level* level) : Entity(level, EntityType::Mob) {
  t_near_entities = new std::vector<bvh::index_t>();
  currentState = new WanderState();
};

Mob::~Mob() {
  t_near_entities->clear();
  t_near_entities->shrink_to_fit();
  delete t_near_entities;
};

void Mob::fixedUpdate(const float& fixedDeltaTime) {
  if (currentState != nullptr) {
    currentState->update(this, fixedDeltaTime);
  }
}

void Mob::update(const float& deltaTime) {}

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

  Vec4 nextXZPos = getNextXZPosition(fixedDeltaTime, target);
  updateXZPosition(fixedDeltaTime, nextXZPos);

  if (position.distanceTo(target) <= 0.15f) {
    currentPath->currentIndex++;

    // If it was the the final step, return true.
    if (currentPath->currentIndex >= currentPath->waypoints.size()) {
      onStopMoving();
      return true;
    }
  }

  return false;
}