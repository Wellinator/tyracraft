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

Pig::Pig(Renderer* t_renderer, SoundManager* t_soundManager,
         ChunckManager* t_chunkManager, Texture* pigTexture,
         DynamicMesh* baseMesh)
    : PassiveMob(MobType::Pig) {
  this->t_renderer = t_renderer;
  this->t_soundManager = t_soundManager;
  this->t_chunkManager = t_chunkManager;
  this->texture = pigTexture;

  loadMesh(baseMesh);
  loadStaticBBox();

  isWalkingAnimationSet = false;
  isStandStillAnimationSet = false;

  isOnGround = true;
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

void Pig::update(const float& deltaTime, const Vec4& movementDir,
                 LevelMap* t_terrain) {
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

  Vec4 min, max;
  BBox currentBbox = getHitBox();
  currentBbox.getMinMax(&min, &max);

  // Update updateStateInWater every 5 ticks
  // if (isTicksCounterAt(5)) updateStateInWater(t_terrain, &min, &max);

  isMoving = movementDir.length() > 0;

  if (isMoving) {
    Vec4 nextPosition = getNextPosition(deltaTime, movementDir);
    if (nextPosition.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS)) {
      const bool hasChangedPosition =
          updatePosition(deltaTime, nextPosition, &currentBbox, &min, &max);

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

  // TODO: add direction
  mesh->rotation.identity();
  float revTheta =
      Utils::reverseAngle(Tyra::Math::atan2(movementDir.x, movementDir.z));
  mesh->rotation.rotateY(revTheta);

  updateTerrainHeightAtEntityPosition(position, &min, &max);
  const Vec4 nextYPos = getNextVrticalPosition(deltaTime);
  updateGravity(nextYPos, &currentBbox, &min, &max);

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

  Vec4 _direction = direction * Vec4(1, 0, 1);
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

  const float enityHeight = std::abs(bbox->getHeight());

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

  float finalHitDistance = -1.0f;
  float tempHitDistance = -1.0f;
  const float maxCollidableDistance = currentPos.distanceTo(nextPosition);

  // Prepate the raycast
  const Vec4 segmentStart = rayOrigin;
  const Vec4 segmentEnd = (rayDir * (maxCollidableDistance * 2)) + rayOrigin;

  // Broad phase
  std::vector<index_t> ni;
  g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);

    if (entityBB->getBottomFace().axisPosition >= entity->maxCorner.y ||
        entityBB->getTopFace().axisPosition < entity->minCorner.y)
      continue;

    const Ray ray = Ray(rayOrigin, rayDir);

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(*entityMin, *entityMax, entity->minCorner,
                           entity->maxCorner, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      if (tempHitDistance > maxCollidableDistance) continue;

      if (finalHitDistance == -1.0f || tempHitDistance < finalHitDistance) {
        finalHitDistance = tempHitDistance;
      }
    }
  }

  // Will collide somewhere?
  if (finalHitDistance > -1.0f) {
    const float timeToHit = finalHitDistance / this->speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        finalHitDistance < position.distanceTo(nextPosition)) {
      if (isColliding) return false;

      // Try to move in separated axis;
      Vec4 moveOnXOnly = Vec4(nextPosition.x, currentPos.y, currentPos.z);
      if (updatePosition(deltaTime, moveOnXOnly, entityBB, entityMin, entityMax,
                         1))
        return true;

      Vec4 moveOnZOnly = Vec4(currentPos.x, currentPos.y, nextPosition.z);
      if (updatePosition(deltaTime, moveOnZOnly, entityBB, entityMin, entityMax,
                         1))
        return true;

      return false;
    }
  }

  // Apply new position;
  position.set(nextPosition);
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

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);

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
  const float width = DUBLE_BLOCK_SIZE * 0.7F;
  const float depth = DUBLE_BLOCK_SIZE * 0.5F;
  const float height = DUBLE_BLOCK_SIZE;

  Vec4 minCorner = Vec4(-width, 0, -depth);
  Vec4 maxCorner = Vec4(width, height, depth);

  Vec4 vertices[8] = {Vec4(minCorner),
                      Vec4(maxCorner.x, minCorner.y, minCorner.z),
                      Vec4(minCorner.x, maxCorner.y, minCorner.z),
                      Vec4(minCorner.x, minCorner.y, maxCorner.z),
                      Vec4(maxCorner),
                      Vec4(minCorner.x, maxCorner.y, maxCorner.z),
                      Vec4(maxCorner.x, minCorner.y, maxCorner.z),
                      Vec4(maxCorner.x, maxCorner.y, minCorner.z)};

  bbox = new BBox(vertices, 8);

  bvh::AABB blockAABB = bvh::AABB();
  blockAABB.minx = minCorner.x;
  blockAABB.miny = minCorner.y;
  blockAABB.minz = minCorner.z;
  blockAABB.maxx = maxCorner.x;
  blockAABB.maxy = maxCorner.y;
  blockAABB.maxz = maxCorner.z;
  tree_index = g_AABBTree->insert(blockAABB, this);
}

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

void Pig::updateStateInWater(LevelMap* terrain, Vec4* min, Vec4* max) {
  Vec4 mid, top, bottom;
  mid = ((*max - *min) / 2) + *min;

  bottom.set(mid.x, min->y, mid.z);
  top.set(mid.x, max->y + 6.0F, mid.z);

  auto blockBottom =
      static_cast<Blocks>(getBlockByWorldPosition(terrain, &bottom));

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

  int ch = t_soundManager->getAvailableChannel();
  SfxLibrarySound* sound =
      t_soundManager->getSound(SoundFxCategory::Mob, stepSfx);

  SfxConfigModel config;
  config._volume = 15;
  config._pitch = 100;

  t_soundManager->setSfxVolume(config._volume, ch);
  t_soundManager->playSfx(sound, ch);
}

void Pig::playSaySfx() {
  const std::array<SoundFX, 3> availableSaySounds = {
      SoundFX::PigSay1,
      SoundFX::PigSay2,
      SoundFX::PigSay3,
  };

  const auto index = Tyra::Math::randomi(0, 2);
  SoundFX saySfx = availableSaySounds[index];

  int ch = t_soundManager->getAvailableChannel();
  SfxLibrarySound* sound =
      t_soundManager->getSound(SoundFxCategory::Mob, saySfx);

  SfxConfigModel config;
  config._volume = 100;
  config._pitch = Tyra::Math::randomi(80, 120);

  t_soundManager->setSfxVolume(config._volume, ch);
  t_soundManager->playSfx(sound, ch);
}

void Pig::playDeathSfx() {
  int ch = t_soundManager->getAvailableChannel();
  SfxLibrarySound* sound =
      t_soundManager->getSound(SoundFxCategory::Mob, SoundFX::PigDeath);

  SfxConfigModel config;
  config._volume = 100;
  config._pitch = Tyra::Math::randomi(80, 120);

  t_soundManager->setSfxVolume(config._volume, ch);
  t_soundManager->playSfx(sound, ch);
}