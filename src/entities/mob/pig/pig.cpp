#include "entities/mob/pig/pig.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "managers/collision_manager.hpp"
#include "3libs/bvh/bvh.h"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer3D;

// ----
// Constructors/Destructors
// ----

Pig::Pig(Level* level, Renderer* t_renderer, ChunckManager* t_chunkManager,
         Texture* pigTexture, DynamicMesh* baseMesh)
    : Mob(level) {
  pLevel = level;
  this->t_renderer = t_renderer;
  this->t_chunkManager = t_chunkManager;
  this->texture = pigTexture;

  loadMesh(baseMesh);
  loadStaticBBox();

  isWalkingAnimationSet = false;
  isStandStillAnimationSet = false;

  isOnGround = true;
  collidable = true;
}

Pig::~Pig() {
  underEntity = nullptr;
  overEntity = nullptr;

  g_AABBTree->remove(tree_index);

  for (size_t i = 0; i < mesh->materials.size(); i++)
    texture->removeLinkById(mesh->materials[i]->id);

  delete bbox;

  walkSequence.clear();
  walkSequence.shrink_to_fit();

  standStillSequence.clear();
  standStillSequence.shrink_to_fit();
}

// ----
// Methods
// ----

void Pig::update(const float& deltaTime, const Vec4& movementDir) {
  u8 fullProcessing = true;
  if (currentChunck) {
    if (currentChunck->state != ChunkState::Loaded ||
        currentChunck->getDistanceFromPlayerInChunks() > 6) {
      shouldUnspawn = true;
      return;
    } else if (currentChunck->getDistanceFromPlayerInChunks() > 3) {
      // Too faraway, no updates
      return;
    } else if (currentChunck->getDistanceFromPlayerInChunks() == 3) {
      // no sounds and larger ticks updates
      fullProcessing = false;
    }
  }

  isMoving = movementDir.length() > 0;

  if (isMoving) {
    // TODO: add direction
    // Update mesh rotation; This routine do not change the hitbox
    mesh->rotation.identity();
    float revTheta =
        Utils::reverseAngle(Tyra::Math::atan2(movementDir.x, movementDir.z));
    mesh->rotation.rotateY(revTheta);
  }

  Vec4 min, max;
  BBox tempBBox = getHitBox(&min, &max);

  // Update  State In Water every 5 ticks
  if (isTicksCounterAt(5)) updateStateInWater(&min, &max);

  if (isMoving) {
    Vec4 nextPosition = getNextPosition(deltaTime, movementDir);

    if (nextPosition.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS)) {
      const bool hasChangedPosition =
          updatePosition(deltaTime, nextPosition, &tempBBox, &min, &max);

      if (hasChangedPosition) {
        if (isWalkingAnimationSet)
          updateWalkingAnimationSpeed();
        else
          setWalkingAnimation();

        // TODO:
        // if (isOnWater()) {
        // playSwimSfx();
        // }

        if (fullProcessing) {
          if (canPlayStepSfx()) {
            playStepSfx();
            lastTimePlayedStepSfx = 0.0F;
            stepSfxLimit = Tyra::Math::randomf(0.25f, 2.0f);
          } else {
            lastTimePlayedStepSfx += deltaTime;
          }
        }

        currentChunck = t_chunkManager->getChunckByWorldPosition(nextPosition);
      } else if (isWalkingAnimationSet) {
        unsetWalkingAnimation();
      }
    }
  } else {
    // Deaccelerate entity speed
    if (speed > 0) {
      speed -= acceleration * deltaTime;
    } else if (speed < 0) {
      speed = 0;
    }

    if (isWalkingAnimationSet) {
      unsetWalkingAnimation();
    }
  }

  if (fullProcessing) {
    if (canPlaySaySfx()) {
      playSaySfx();
      lastTimePlayedSaySfx = 0;
      saySfxLimit = Tyra::Math::randomf(5.0f, 20.0f);
    } else {
      lastTimePlayedSaySfx += deltaTime;
    }
  }

  updateTerrainHeightAtEntityPosition(position, &min, &max);
  const Vec4 nextYPos = getNextVrticalPosition(deltaTime);
  updateGravity(nextYPos, &tempBBox, &min, &max);

  // Update entity bbox
  delete bbox;
  bbox = new BBox(tempBBox);

  mesh->getPosition()->set(position);
  mesh->update();
}

Vec4 Pig::getNextPosition(const float& deltaTime, const Vec4& direction) {
  const float _maxSpeed = maxSpeed;
  const float _maxAcc = acceleration;

  // Accelerate speed until mach max
  if (speed < _maxSpeed) {
    speed += _maxAcc * deltaTime;
  } else if (speed > _maxSpeed) {
    // Deaccelerate speed to new max
    speed -= _maxAcc * deltaTime;
    if (speed < _maxSpeed) speed = _maxSpeed;
  }

  Vec4 _direction = Vec4(direction.x, 0.0F, direction.z);
  Vec4 result = _direction * (speed * deltaTime);

  if (_isOnWater) {
    result *= IN_WATER_FRICTION;
  }

  return result + position;
}

Vec4 Pig::getNextVrticalPosition(const float& deltaTime) {
  // Accelerate the velocity: velocity += gravConst * deltaTime
  velocity += Vec4(velocity.x, GRAVITY.y * deltaTime, velocity.z);

  if (_isOnWater) {
    velocity.y *= GRAVITY_ON_WATER_FACTOR;
  }

  // Increase the position by velocity
  Vec4 nextVerticalPosition = position + (velocity * deltaTime);
  return nextVerticalPosition;
}

/** Update entity position by gravity and update index of current block */
void Pig::updateGravity(const Vec4 nextVerticalPosition, BBox* bbox,
                        Vec4* entityMin, Vec4* entityMax) {
  Vec4 newPosition = nextVerticalPosition;

  const float worldMinHeight = OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE;
  const float worldMaxHeight = OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE;

  if (entityMin->y > worldMaxHeight || entityMax->y < worldMinHeight) {
    velocity = Vec4(0.0f, 0.0f, 0.0f);
    shouldUnspawn = true;
    return;
  }

  const float enityHeight = Utils::Abs(DUBLE_BLOCK_SIZE * 0.9F);

  if (newPosition.y < terrainHeight.minHeight) {
    newPosition.y = terrainHeight.minHeight;
    velocity = Vec4(0.0f, 0.0f, 0.0f);
    isOnGround = true;
  } else if (newPosition.y + enityHeight >= terrainHeight.maxHeight) {
    newPosition.y = terrainHeight.maxHeight;
    velocity = -velocity;
    isOnGround = false;
  }

  // Finally updates gravity after checks
  position.set(newPosition);
}

u8 Pig::updatePosition(const float& deltaTime, const Vec4& nextPosition,
                       BBox* entityBB, Vec4* entityMin, Vec4* entityMax,
                       u8 isColliding) {
  Vec4 currentPos = position;

  // Set ray props
  Vec4 rayOrigin = ((*entityMax - *entityMin) / 2) + *entityMin;
  Vec4 rayDir = (nextPosition - currentPos).getNormalized();

  Ray ray;
  float finalHitDistance = -1.0f;
  float tempHitDistance = -1.0f;
  const float maxCollidableDistance = currentPos.distanceTo(nextPosition);

  // Broad phase
  // std::vector<index_t> ni;

  // Prepate the raycast
  const Vec4 segmentStart = rayOrigin;
  const Vec4 segmentEnd = (rayDir * (maxCollidableDistance)) + rayOrigin;

  t_near_entities->clear();
  g_AABBTree->intersectLine(segmentStart, segmentEnd, *t_near_entities);
  t_near_entities->shrink_to_fit();

  bvh::AABB tempAABB = bvh::AABB();
  tempAABB.minx = entityMin->x;
  tempAABB.miny = entityMin->y;
  tempAABB.minz = entityMin->z;
  tempAABB.maxx = entityMax->x;
  tempAABB.maxy = entityMax->y;
  tempAABB.maxz = entityMax->z;

  // t_near_entities->clear();
  // g_AABBTree->find_overlaps(tempAABB, *t_near_entities);
  // t_near_entities->shrink_to_fit();

  t_renderer->renderer3D.utility.drawLine(segmentStart, segmentEnd,
                                          Color(255, 0, 0));

  for (u16 i = 0; i < t_near_entities->size(); i++) {
    Entity* entity =
        reinterpret_cast<Entity*>(g_AABBTree->user_data((*t_near_entities)[i]));
    if (!entity) {
      TYRA_TRAP("Invalit entity!");
      return false;
    };

    if (!entity->collidable) continue;

    if (entityBB->getBottomFace().axisPosition >= entity->maxCorner.y ||
        entityBB->getTopFace().axisPosition < entity->minCorner.y)
      continue;

    ray.origin.set(rayOrigin);
    ray.direction.set(rayDir);

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(*entityMin, *entityMax, entity->minCorner,
                           entity->maxCorner, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      // if (tempHitDistance > maxCollidableDistance) continue;

      if (finalHitDistance == -1.0f || tempHitDistance < finalHitDistance) {
        finalHitDistance = tempHitDistance;
      }
    }
  }

  // Will collide somewhere?
  if (finalHitDistance > -1.0f) {
    const double timeToHit = finalHitDistance / speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        finalHitDistance < position.distanceTo(nextPosition)) {
      if (isColliding) return false;

      // Try to move in separated axis;
      Vec4 moveOnXOnly = Vec4(nextPosition.x, currentPos.y, currentPos.z);
      Vec4 moveOnZOnly = Vec4(currentPos.x, currentPos.y, nextPosition.z);

      return updatePosition(deltaTime, moveOnXOnly, entityBB, entityMin,
                            entityMax, true) ||
             updatePosition(deltaTime, moveOnZOnly, entityBB, entityMin,
                            entityMax, true);
    }
  }

  // Apply new position;
  position.set(nextPosition);

  // Update the aabb in the bvh tree
  g_AABBTree->move(tree_index, tempAABB);

  return true;
}

void Pig::updateTerrainHeightAtEntityPosition(const Vec4 nextVrticalPosition,
                                              Vec4* minEntityPos,
                                              Vec4* maxEntityPos) {
  terrainHeight.reset();
  underEntity = nullptr;
  overEntity = nullptr;

  // Prepate the raycast
  const Vec4 offset = Vec4(0, 40, 0);
  const Vec4 segmentStart = *maxEntityPos + offset;
  const Vec4 segmentEnd = *minEntityPos - offset;

  std::vector<int32_t> ni;
  g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

  // TYRA_LOG("ni: ", (int)ni.size());

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = reinterpret_cast<Entity*>(g_AABBTree->user_data(ni[i]));
    if (!entity->collidable) continue;

    // is under or above block
    if (minEntityPos->x <= entity->maxCorner.x &&
        maxEntityPos->x >= entity->minCorner.x &&
        minEntityPos->z <= entity->maxCorner.z &&
        maxEntityPos->z >= entity->minCorner.z) {
      const float minHeight = entity->maxCorner.y;
      if (minEntityPos->y >= minHeight && minHeight > terrainHeight.minHeight) {
        terrainHeight.minHeight = minHeight;
        underEntity = entity;
      }

      const float maxHeight = entity->minCorner.y;
      if (maxEntityPos->y < maxHeight && maxHeight < terrainHeight.maxHeight) {
        terrainHeight.maxHeight = maxHeight;
        overEntity = entity;
      }
    }
  }
}

void Pig::loadMesh(DynamicMesh* baseMesh) {
  mesh = new DynamicMesh(*baseMesh);
  mesh->rotation.identity();
  mesh->scale.identity();
  // mesh->scale.scaleX(0.85F);

  auto& materials = mesh->materials;
  for (size_t i = 0; i < materials.size(); i++)
    texture->addLink(materials[i]->id);

  mesh->animation.setSequence(standStillSequence);
  mesh->animation.loop = false;
  mesh->animation.speed = 0;
}

void Pig::loadStaticBBox() {
  if (bbox) delete bbox;

  Vec4 minCorner, maxCorner;
  bbox = new BBox(getHitBox(&minCorner, &maxCorner));

  bvh::AABB blockAABB = bvh::AABB();
  blockAABB.minx = minCorner.x;
  blockAABB.miny = minCorner.y;
  blockAABB.minz = minCorner.z;
  blockAABB.maxx = maxCorner.x;
  blockAABB.maxy = maxCorner.y;
  blockAABB.maxz = maxCorner.z;
  tree_index = g_AABBTree->insert(blockAABB, this);
}

BBox Pig::getHitBox(Vec4* t_min, Vec4* t_max) const {
  // It's a sqr save value for Width, Depth and Height
  const float size = DUBLE_BLOCK_SIZE * 0.9F;
  const float halfSize = (DUBLE_BLOCK_SIZE * 0.9F) / 2;

  Vec4 minCorner = Vec4(-halfSize, 0, -halfSize) + position;
  Vec4 maxCorner = Vec4(halfSize, size, halfSize) + position;

  if (t_min) t_min->set(minCorner);
  if (t_max) t_max->set(maxCorner);

  Vec4 vertices[8] = {Vec4(minCorner),
                      Vec4(maxCorner.x, minCorner.y, minCorner.z),
                      Vec4(minCorner.x, maxCorner.y, minCorner.z),
                      Vec4(minCorner.x, minCorner.y, maxCorner.z),
                      Vec4(maxCorner),
                      Vec4(minCorner.x, maxCorner.y, maxCorner.z),
                      Vec4(maxCorner.x, minCorner.y, maxCorner.z),
                      Vec4(maxCorner.x, maxCorner.y, minCorner.z)};

  return BBox(vertices, 8);
};

void Pig::jump() {
  velocity += lift;
  isOnGround = false;
}

void Pig::swim() {
  if (_isOnWater) {
    velocity += (lift * 0.25F);
    _isOnWater = false;
  }

  if (velocity.y > lift.y) velocity.y = lift.y;

  isOnGround = false;
}

void Pig::setWalkingAnimation() {
  mesh->animation.setSequence(walkSequence);
  mesh->animation.speed = 0.3f / speed;
  mesh->animation.loop = true;
  isWalkingAnimationSet = true;
}

void Pig::updateWalkingAnimationSpeed() {
  mesh->animation.speed = 0.3f / speed;
}

void Pig::unsetWalkingAnimation() {
  mesh->animation.setSequence(standStillSequence);
  mesh->animation.loop = false;
  isWalkingAnimationSet = false;
}

void Pig::updateStateInWater(Vec4* min, Vec4* max) {
  Vec4 mid, top, bottom;
  mid = ((*max - *min) / 2) + *min;

  bottom.set(mid.x, min->y, mid.z);
  top.set(mid.x, max->y + 6.0F, mid.z);

  auto blockBottom =
      static_cast<Blocks>(pLevel->getBlockByWorldPosition(&bottom));

  _isOnWater = blockBottom == Blocks::WATER_BLOCK;
}

bool Pig::isOnWater() { return _isOnWater; }

void Pig::playStepSfx() {
  const std::array<SoundFX, 5> availableStepSounds = {
      SoundFX::PigStep1, SoundFX::PigStep2, SoundFX::PigStep3,
      SoundFX::PigStep4, SoundFX::PigStep5,
  };

  const auto index = Tyra::Math::randomi(0, 4);
  SoundFX stepSfx = availableStepSounds[index];

  SoundManager* pSoundManager = SoundManager::getInstance();
  int ch = pSoundManager->getAvailableChannel();
  SfxLibrarySound* sound =
      pSoundManager->getSound(SoundFxCategory::Mob, stepSfx);

  SfxConfigModel config;
  config._volume = 15;
  config._pitch = 100;

  pSoundManager->setSfxVolume(config._volume, ch);
  pSoundManager->playSfx(sound, ch);
}

void Pig::playSaySfx() {
  const std::array<SoundFX, 3> availableSaySounds = {
      SoundFX::PigSay1,
      SoundFX::PigSay2,
      SoundFX::PigSay3,
  };

  const auto index = Tyra::Math::randomi(0, 2);
  SoundFX saySfx = availableSaySounds[index];

  SoundManager* pSoundManager = SoundManager::getInstance();
  int ch = pSoundManager->getAvailableChannel();
  SfxLibrarySound* sound =
      pSoundManager->getSound(SoundFxCategory::Mob, saySfx);

  SfxConfigModel config;
  config._volume = 100;
  config._pitch = Tyra::Math::randomi(80, 120);

  pSoundManager->setSfxVolume(config._volume, ch);
  pSoundManager->playSfx(sound, ch);
}

void Pig::playDeathSfx() {
  SoundManager* pSoundManager = SoundManager::getInstance();
  int ch = pSoundManager->getAvailableChannel();
  SfxLibrarySound* sound =
      pSoundManager->getSound(SoundFxCategory::Mob, SoundFX::PigDeath);

  SfxConfigModel config;
  config._volume = 100;
  config._pitch = Tyra::Math::randomi(80, 120);

  pSoundManager->setSfxVolume(config._volume, ch);
  pSoundManager->playSfx(sound, ch);
}

// ----
// Override
// ----
/** Mob category */
MobCategory Pig::getCategory() { return MobCategory::Passive; }

/** Mob type */
MobType Pig::getType() { return MobType::Pig; }