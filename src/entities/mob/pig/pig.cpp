#include "entities/mob/pig/pig.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "managers/collision_manager.hpp"
#include "3libs/bvh/bvh.h"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
using Tyra::Renderer3D;

// ----
// Constructors/Destructors
// ----

Pig::Pig(Renderer* t_renderer, SoundManager* t_soundManager,
         Texture* pigTexture)
    : PassiveMob(MobType::Pig) {
  this->t_renderer = t_renderer;
  this->t_soundManager = t_soundManager;
  this->texture = pigTexture;

  loadMesh();
  loadStaticBBox();

  isWalkingAnimationSet = false;
  isStandStillAnimationSet = false;

  isOnGround = true;
}

Pig::~Pig() {
  underEntity = nullptr;
  overEntity = nullptr;

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
  // Update updateStateInWater every 5 ticks
  if (isTicksCounterAt(5)) updateStateInWater(t_terrain);

  isMoving = movementDir.length() > 0;

  if (isMoving) {
    Vec4 nextPosition = getNextPosition(deltaTime, movementDir);
    if (nextPosition.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS)) {
      const bool hasChangedPosition = updatePosition(deltaTime, nextPosition);

      if (hasChangedPosition) {
        if (lastTimePlayedSfx > 0.35) {
          if (isOnWater()) {
            // TODO:
            // playSwimSfx();
          }

          setWalkingAnimation();
          lastTimePlayedSfx = 0.0F;
        } else {
          lastTimePlayedSfx += deltaTime;
        }
      }
    }
  } else {
    // Deaccelerate entity speed
    if (speed > 0) {
      speed -= acceleration * deltaTime;
    } else if (speed < 0) {
      speed = 0;
    }
    unsetWalkingAnimation();
  }

  // TODO: add direction
  mesh.get()->rotation.identity();
  float revTheta =
      Utils::reverseAngle(Tyra::Math::atan2(movementDir.x, movementDir.z));
  mesh->rotation.rotateY(revTheta);

  const Vec4 nextYPos = getNextVrticalPosition(deltaTime);
  updateTerrainHeightAtEntityPosition(nextYPos);
  updateGravity(nextYPos);

  animate();
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

  return result + *mesh->getPosition();
}

Vec4 Pig::getNextVrticalPosition(const float& deltaTime) {
  // Accelerate the velocity: velocity += gravConst * deltaTime
  velocity += Vec4(velocity.x, GRAVITY.y * deltaTime, velocity.z);

  if (_isOnWater) {
    velocity.y *= GRAVITY_ON_WATER_FACTOR;
  }

  // Increase the position by velocity
  Vec4 nextVerticalPosition = *mesh->getPosition() + (velocity * deltaTime);
  return nextVerticalPosition;
}

/** Update entity position by gravity and update index of current block */
void Pig::updateGravity(const Vec4 nextVerticalPosition) {
  Vec4 newPosition = nextVerticalPosition;
  const float worldMinHeight = OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE;
  const float worldMaxHeight = OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE;

  if (newPosition.y + bbox->getHeight() > worldMaxHeight ||
      newPosition.y < worldMinHeight) {
    // Maybe has died, teleport to spaw area
    TYRA_LOG("\nReseting entity position to:\n");
    mesh->getPosition()->set(spawnPosition);
    velocity = Vec4(0.0f, 0.0f, 0.0f);
    return;
  }

  const float enityHeight = std::abs(bbox->getHeight());
  const float heightLimit = terrainHeight.maxHeight - enityHeight;

  if (newPosition.y < terrainHeight.minHeight) {
    newPosition.y = terrainHeight.minHeight;
    velocity = Vec4(0.0f, 0.0f, 0.0f);
    isOnGround = true;
  } else if (newPosition.y >= heightLimit) {
    newPosition.y = heightLimit;
    velocity = -velocity;
    isOnGround = false;
  }

  // Finally updates gravity after checks
  mesh->getPosition()->set(newPosition);
}

u8 Pig::updatePosition(const float& deltaTime, const Vec4& nextPosition,
                       u8 isColliding) {
  Vec4 currentPlayerPos = *this->mesh->getPosition();
  Vec4 entityMin;
  Vec4 entityMax;
  BBox entityBB = getHitBox();
  entityBB.getMinMax(&entityMin, &entityMax);

  // Set ray props
  Vec4 rayOrigin = ((entityMax - entityMin) / 2) + entityMin;
  Vec4 rayDir = (nextPosition - currentPlayerPos).getNormalized();

  float finalHitDistance = -1.0f;
  float tempHitDistance = -1.0f;
  const float maxCollidableDistance = currentPlayerPos.distanceTo(nextPosition);

  // Prepate the raycast
  const Vec4 segmentStart = rayOrigin;
  const Vec4 segmentEnd = (rayDir * (maxCollidableDistance * 2)) + rayOrigin;

  // Broad phase
  std::vector<index_t> ni;
  g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);

    if (entityBB.getBottomFace().axisPosition >= entity->maxCorner.y ||
        entityBB.getTopFace().axisPosition < entity->minCorner.y)
      continue;

    const Ray ray = Ray(rayOrigin, rayDir);

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(entityMin, entityMax, entity->minCorner,
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
        finalHitDistance <
            this->mesh->getPosition()->distanceTo(nextPosition)) {
      if (isColliding) return false;

      // Try to move in separated axis;
      Vec4 moveOnXOnly =
          Vec4(nextPosition.x, currentPlayerPos.y, currentPlayerPos.z);
      if (updatePosition(deltaTime, moveOnXOnly, 1)) return true;

      Vec4 moveOnZOnly =
          Vec4(currentPlayerPos.x, currentPlayerPos.y, nextPosition.z);
      if (updatePosition(deltaTime, moveOnZOnly, 1)) return true;

      return false;
    }
  }

  // Apply new position;
  mesh->getPosition()->set(nextPosition);
  return true;
}

void Pig::updateTerrainHeightAtEntityPosition(const Vec4 nextVrticalPosition) {
  BBox entityBB = this->getHitBox();
  Vec4 minEntityPos, maxEntityPos;
  entityBB.getMinMax(&minEntityPos, &maxEntityPos);

  terrainHeight.reset();
  underEntity = nullptr;
  overEntity = nullptr;

  // Prepate the raycast
  const Vec4 offset = Vec4(0, 40, 0);
  const Vec4 segmentStart = maxEntityPos + offset;
  const Vec4 segmentEnd = nextVrticalPosition - offset;

  std::vector<int32_t> ni;
  g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);

    // is under or above block
    if (minEntityPos.x <= entity->maxCorner.x &&
        maxEntityPos.x >= entity->minCorner.x &&
        minEntityPos.z <= entity->maxCorner.z &&
        maxEntityPos.z >= entity->minCorner.z) {
      const float minHeight = entity->maxCorner.y;
      if (minEntityPos.y >= minHeight && minHeight > terrainHeight.minHeight) {
        terrainHeight.minHeight = minHeight;
        underEntity = entity;
      }

      const float maxHeight = entity->minCorner.y;
      if (maxEntityPos.y < maxHeight && maxHeight < terrainHeight.maxHeight) {
        terrainHeight.maxHeight = maxHeight;
        overEntity = entity;
      }
    }
  }
}

void Pig::loadMesh() {
  ObjLoaderOptions options;
  options.scale = 17.0F;
  options.flipUVs = true;
  options.animation.count = 3;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("models/pig/pig.obj"), options);
  data.get()->loadNormals = false;

  this->mesh = std::make_unique<DynamicMesh>(data.get());

  this->mesh->rotation.identity();
  this->mesh->scale.identity();
  this->mesh->scale.scaleX(0.85F);

  auto& materials = this->mesh.get()->materials;
  for (size_t i = 0; i < materials.size(); i++)
    texture->addLink(materials[i]->id);

  this->mesh->animation.loop = true;
  this->mesh->animation.setSequence(standStillSequence);
  this->mesh->animation.speed = 0.08F;
}

void Pig::loadStaticBBox() {
  const float width = DUBLE_BLOCK_SIZE * 1.8F;
  const float depth = (DUBLE_BLOCK_SIZE * 0.4F) / 2;
  const float height = DUBLE_BLOCK_SIZE * 0.45F;

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
  const float _speed = speed * 10;
  this->mesh->animation.speed = baseAnimationSpeed * _speed;

  if (!isWalkingAnimationSet) {
    this->mesh->animation.setSequence(walkSequence);
    isWalkingAnimationSet = true;
  }
}

void Pig::unsetWalkingAnimation() {
  if (!isWalkingAnimationSet) return;

  isWalkingAnimationSet = false;
  mesh->animation.setSequence(standStillSequence);
}

void Pig::animate() { this->mesh->update(); }

void Pig::updateStateInWater(LevelMap* terrain) {
  Vec4 min, mid, max, top, bottom;
  BBox bbox = getHitBox();
  bbox.getMinMax(&min, &max);
  mid = ((max - min) / 2) + min;

  bottom.set(mid.x, min.y, mid.z);
  top.set(mid.x, max.y + 6.0F, mid.z);

  auto blockBottom =
      static_cast<Blocks>(getBlockByWorldPosition(terrain, &bottom));

  _isOnWater = blockBottom == Blocks::WATER_BLOCK;
}

bool Pig::isOnWater() { return _isOnWater; }