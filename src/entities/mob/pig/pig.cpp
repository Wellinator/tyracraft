#include "entities/mob/pig/pig.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "managers/collision_manager.hpp"
#include "managers/model_builder.hpp"
#include "3libs/bvh/bvh.h"
#include "timer.hpp"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer3D;

// ----
// Constructors/Destructors
// ----

Pig::Pig(Level* level, Renderer* pRenderer, Texture* pigTexture,
         Tyra::Mesh** framesArray, const int size)
    : Mob(level), Animated(framesArray, size) {
  pLevel = level;
  t_renderer = pRenderer;
  texture = pigTexture;

  statPip.setRenderer(&t_renderer->core);
  loadStaticBBox();

  std::vector<u8> standStillSequence = {0};
  AnimationOptions idleAnimation;
  idleAnimation.animationId = IDLE_ANIMATION;
  idleAnimation.framesIndices = standStillSequence;
  idleAnimation.durationInMs = 250.0f;
  idleAnimation.loop = true;
  idleAnimation.wrapFrames = false;
  addAnimation(idleAnimation);

  std::vector<u8> walkSequence = {1, 2};
  AnimationOptions walkAnimation;
  walkAnimation.animationId = WALK_ANIMATION;
  walkAnimation.framesIndices = walkSequence;
  walkAnimation.durationInMs = 700.0f;
  walkAnimation.loop = true;
  walkAnimation.wrapFrames = false;
  addAnimation(walkAnimation);

  setIdleAnimation();

  isWalkingAnimationSet = false;
  isStandStillAnimationSet = false;

  isOnGround = true;
  collidable = true;
}

Pig::~Pig() {
  g_AABBTree->remove(tree_index);
  delete bbox;
}

// ----
// Methods
// ----

void Pig::fixedUpdate(const float& fixedDeltaTime) {
  // Reset lerp state
  // set new prev to old target
  _prevPosition.set(_targetPosition);

  auto pChunkManager = ChunkManager::getInstance();
  Chunk* chk = pChunkManager->getChunkByWorldPosition(position);
  if (chk != nullptr) {
    if (chk->state != ChunkState::Loaded ||
        chk->getDistanceFromPlayerInChunks() > 10) {
      shouldUnspawn = true;
      return;
    }
  }

  Mob::fixedUpdate(fixedDeltaTime);

  // Vertical collision and movement
  const float nextYPos = getNextVrticalPosition(fixedDeltaTime);
  updateTerrainHeightAtEntityPosition();
  updateYPosition(nextYPos);
}

void Pig::update(const float& deltaTime) {
  position.lerp(_prevPosition, _targetPosition, TyraCraft::Timer::stateLerp);
  Mob::update(deltaTime);
  Animated::update(deltaTime);
}

void Pig::render() {
  std::vector<Vec4> vertices = {};
  std::vector<Color> verticesColors = {};
  std::vector<Vec4> uvMap = {};

  // Calc draw data by frames interpolation
  const AnimationOptions currentAnimationOptions = getCurrentAnimation();
  if (currentAnimationOptions.framesIndices.size() > 1) {
    fillDrawDataByLerp(&vertices, &verticesColors, &uvMap);
  } else {
    fillDrawDataByFrame(&vertices, &verticesColors, &uvMap);
  }

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  textureBag.coordinates = uvMap.data();
  textureBag.texture = texture;

  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  Color tempColor = Color(128, 128, 128);
  colorBag.single = &tempColor;  // verticesColors.data();

  M4x4 rawMatrix = M4x4::Identity;
  rawMatrix.rotate(rotation);
  rawMatrix.translate(position);
  infoBag.model = &rawMatrix;

  bag.count = vertices.size();
  bag.vertices = vertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  t_renderer->renderer3D.usePipeline(statPip);
  statPip.core.render(&bag);
}

void Pig::tick() {
  // Update updateStateInWater every 5 ticks
  if (isTicksCounterAt(5)) updateStateInWater();
}

float Pig::getNextVrticalPosition(const float& fixedDeltaTime) {
  velocity.y += GRAVITY.y * fixedDeltaTime;

  if (_isUnderWater) {
    velocity.y *= GRAVITY_UNDER_WATER_FACTOR;
  } else if (_isOnWater) {
    velocity.y *= GRAVITY_ON_WATER_FACTOR;
  }

  return _targetPosition.y + (velocity.y * fixedDeltaTime);
}

Vec4 Pig::getNextXZPosition(const float& fixedDeltaTime, const Vec4& target) {
  float step = speed * fixedDeltaTime;
  if (_isUnderWater || _isOnWater) step *= IN_WATER_FRICTION;

  Vec4 distance = target - _targetPosition;
  Vec4 dir = distance.getNormalized();

  Vec4 delta = Vec4(dir.x, 0.0F, dir.z) * step;
  velocity.x = delta.x;
  velocity.z = delta.z;

  const float length = distance.length();

  // Avoid overshooting
  if (length == 0.0f || step * step >= length) return target;
  return _targetPosition + delta;
}

bool Pig::updateXZPosition(const float& deltaTime, const Vec4& nextPosition,
                           u8 isColliding) {
  const float maxCollidableDistance = _targetPosition.distanceTo(nextPosition);
  const Vec4 positionDiff = nextPosition - _targetPosition;
  const Vec4 direction = positionDiff.getNormalized();

  float shortestDistance = -1.0f;
  float tempHitDistance = -1.0f;
  u8 autoJump = false;

  // Temp custom model expanded by entity volocity
  M4x4 model = M4x4::Identity;
  Vec4 deltaScale = Vec4(std::abs(positionDiff.x), std::abs(positionDiff.y),
                         std::abs(positionDiff.z));

  const Vec4 hitBoxSize = getHitBoxSize();
  Vec4 scaleFraction = (deltaScale / hitBoxSize);
  model.scale(scaleFraction + Vec4(1.5F, 1.5F, 1.5F));
  model.translate(_targetPosition + (positionDiff * hitBoxSize));

  Vec4 _min, _max;
  BBox tempBBox = getHitBox(model);
  tempBBox.getMinMax(&_min, &_max);

  // Broad phase
  // Until now we have aabb expanded by player velocity
  // Need to check collision with inflated aabb (minkowski sum)
  std::vector<LevelIntersectQueryResult> tempResult = {};
  pLevel->getIntersectedBlocksByAABB(_min, _max, &tempResult);

  // Narrow phase
  Vec4 entityMin, entityMax;
  BBox entityBB = getHitBox();
  entityBB.getMinMax(&entityMin, &entityMax);

  const Vec4 origin = ((entityMax - entityMin) / 2) + entityMin;

  // Prepate the raycast
  const Ray ray = Ray(origin, direction);

  for (size_t i = 0; i < tempResult.size(); i++) {
    Vec4 currentOffset = tempResult[i].offset;
    Blocks currentBlockType = static_cast<Blocks>(tempResult[i].blockType);
    StaticBlockRepository* staticBlockRepo =
        StaticBlockRepository::getInstance();
    Block* templateBlock = nullptr;

    templateBlock = staticBlockRepo->getBlockTemplate(currentBlockType);
    if (templateBlock->isCollidable() == false) continue;

    M4x4 model = ModelBuilder_BuildModel(&currentOffset);
    Vec4 min, max;
    BBox blockBBox = VertexBlockData::getRawBBoxByOffset(&currentOffset)
                         ->getTransformed(model);
    blockBBox.getMinMax(&min, &max);

    if (entityBB.getBottomFace().axisPosition >= max.y ||
        entityBB.getTopFace().axisPosition < min.y)
      continue;

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(entityMin, entityMax, min, max, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      if (tempHitDistance > maxCollidableDistance) continue;

      if (shortestDistance == -1.0f || tempHitDistance < shortestDistance) {
        shortestDistance = tempHitDistance;
        autoJump = (max.y - entityMin.y) <= BLOCK_SIZE;
      }
    }
  }

  // Will collide somewhere?
  if (shortestDistance > -1.0f) {
    const float timeToHit = shortestDistance / speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        shortestDistance < _prevPosition.distanceTo(nextPosition)) {
      if (isColliding) {
        return false;
      }

      // Check if can jump
      if (autoJump && isOnGround) {
        jumpQuickly();
        return true;
      }

      // Try to move in separated axis;
      return updateXZPosition(
                 deltaTime,
                 Vec4(nextPosition.x, _targetPosition.y, _targetPosition.z),
                 true) ||
             updateXZPosition(
                 deltaTime,
                 Vec4(_targetPosition.x, _targetPosition.y, nextPosition.z),
                 true);
    }
  }

  if (_targetPosition.distanceTo(nextPosition) > 0.0f) {
    // Apply new position;
  }

  _targetPosition.x = nextPosition.x;
  _targetPosition.z = nextPosition.z;
  return true;

  /*
  // TODO: The code below must be applied for entity vs entity collision
  bvh::AABB _aabb;
  _aabb.minx = _min.x;
  _aabb.miny = _min.y;
  _aabb.minz = _min.z;
  _aabb.maxx = _max.x;
  _aabb.maxy = _max.y;
  _aabb.maxz = _max.z;

  std::vector<index_t> ni;
  g_AABBTree->find_overlaps(_aabb, ni);

  // Narrow phase
  Vec4 entityMin, entityMax;
  BBox entityBB = getHitBox();
  entityBB.getMinMax(&entityMin, &entityMax);

  const Vec4 origin = ((entityMax - entityMin) / 2) + entityMin;

  // Prepate the raycast
  const Ray ray = Ray(origin, direction);

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = static_cast<Entity*>(g_AABBTree->user_data(ni[i]));
    if (!entity->collidable) continue;

    if (entityBB.getBottomFace().axisPosition >= entity->maxCorner.y ||
        entityBB.getTopFace().axisPosition < entity->minCorner.y)
      continue;

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(entityMin, entityMax, entity->minCorner,
                           entity->maxCorner, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      if (tempHitDistance > maxCollidableDistance) continue;

      if (shortestDistance == -1.0f || tempHitDistance < shortestDistance) {
        shortestDistance = tempHitDistance;
        autoJump = entity->maxCorner.y > entityMin.y &&
                   (entity->maxCorner.y - entityMin.y <= BLOCK_SIZE);
      }
    }
  }

  // Will collide somewhere?
  if (shortestDistance > -1.0f) {
    const float timeToHit = shortestDistance / speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        shortestDistance < _prevPosition.distanceTo(nextPosition)) {
      if (isColliding) {
        return false;
      }

      // Check if can jump
      if (autoJump && isOnGround) {
        jumpQuickly();
        return true;
      }

      // Try to move in separated axis;
      return updateXZPosition(
                 deltaTime,
                 Vec4(nextPosition.x, _targetPosition.y, _targetPosition.z),
                 true) ||
             updateXZPosition(
                 deltaTime,
                 Vec4(_targetPosition.x, _targetPosition.y, nextPosition.z),
                 true);
    }
  }

  if (_targetPosition.distanceTo(nextPosition) > 0.0f) {
    // Apply new position;
    _targetPosition.x = nextPosition.x;
    _targetPosition.z = nextPosition.z;
    return true;
  }

  return false;
  */
}

void Pig::resolveOutOfWorldBoundaries() {
  velocity = Vec4(0.0f, 0.0f, 0.0f);
  shouldUnspawn = true;
  return;
};

const float Pig::getHeight() { return DOUBLE_BLOCK_SIZE * 0.9F; }

void Pig::loadStaticBBox() {
  if (bbox) delete bbox;

  const Vec4 hitBoxSize = getHitBoxSize();
  Vec4 minCorner = Vec4(-hitBoxSize.x, 0, -hitBoxSize.z);
  Vec4 maxCorner = Vec4(hitBoxSize.x, hitBoxSize.y, hitBoxSize.z);

  minCorner += position;
  maxCorner += position;

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

void Pig::jumpQuickly() {
  velocity += lift * 0.75f;
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

void Pig::setWalkingAnimation() { setAnimation(WALK_ANIMATION); }

void Pig::setIdleAnimation() { setAnimation(IDLE_ANIMATION); }

void Pig::updateStateInWater() {
  Vec4 min, mid, max, top, bottom;
  BBox currentBBox = getHitBox();
  currentBBox.getMinMax(&min, &max);

  // mid = ((max - min) / 2) + min;
  mid = currentBBox.getCenter();

  bottom.set(mid.x, min.y, mid.z);
  top.set(mid.x, max.y + 6.0F, mid.z);

  auto blockBottom =
      static_cast<Blocks>(pLevel->getBlockByWorldPosition(&bottom));
  auto blockTop = static_cast<Blocks>(pLevel->getBlockByWorldPosition(&top));

  _isOnWater = blockBottom == Blocks::WATER_BLOCK;
  _isUnderWater = blockTop == Blocks::WATER_BLOCK;

  if (!isSubmerged && _isUnderWater) {
    isSubmerged = true;
    playSplashSfx();
  } else if (isSubmerged && !_isUnderWater) {
    isSubmerged = false;
    playSplashSfx();
  }
}

bool Pig::isOnWater() { return _isOnWater; }

bool Pig::isUnderWater() { return _isUnderWater; }

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

void Pig::playSwimSfx() {
  SoundManager* pSoundManager = SoundManager::getInstance();
  const int ch = pSoundManager->getAvailableChannel();

  auto randSwimSfx = static_cast<SoundFX>(Tyra::Math::randomi(
      static_cast<u8>(SoundFX::Swim1), static_cast<u8>(SoundFX::Swim4)));
  const u8 randPich = Tyra::Math::randomi(60, 140);
  const u8 volume = 30;

  SfxLibrarySound* sound =
      pSoundManager->getSound(SoundFxCategory::Liquid, randSwimSfx);

  sound->_sound->pitch = randPich;
  pSoundManager->setSfxVolume(volume, ch);
  pSoundManager->playSfx(sound, ch);
}

void Pig::playSplashSfx() {
  SoundManager* pSoundManager = SoundManager::getInstance();
  const int ch = pSoundManager->getAvailableChannel();
  auto randSwimSfx = static_cast<SoundFX>(Tyra::Math::randomi(
      static_cast<u8>(SoundFX::Splash), static_cast<u8>(SoundFX::Splash2)));

  const u8 randPich = Tyra::Math::randomi(60, 140);
  const u8 volume = 60;

  SfxLibrarySound* sound =
      pSoundManager->getSound(SoundFxCategory::Liquid, randSwimSfx);
  sound->_sound->pitch = randPich;
  pSoundManager->setSfxVolume(volume, ch);
  pSoundManager->playSfx(sound, ch);
}

void Pig::onMoved() {
  if (lastTimePlayedStepSfx > stepSfxLimit) {
    if (isOnWater() || isUnderWater()) {
      playSwimSfx();
    } else if (isOnGround) {
      playStepSfx();
      lastTimePlayedStepSfx = 0.0F;
      stepSfxLimit = Tyra::Math::randomf(0.25f, 2.0f);
    }

    if (!isWalkingAnimationSet) setWalkingAnimation();

  } else {
    lastTimePlayedStepSfx +=
        TyraCraft::Timer::getInstance()->getFixedDeltaTime();
  }
}

void Pig::onStopMoving() { setIdleAnimation(); }

// ----
// Override
// ----
/** Mob category */
MobCategory Pig::getCategory() { return MobCategory::Passive; }

/** Mob type */
MobType Pig::getType() { return MobType::Pig; }
