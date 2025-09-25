#include "entities/player/player.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "managers/block_manager.hpp"
#include "managers/collision_manager.hpp"
#include "3libs/bvh/bvh.h"
#include "managers/settings_manager.hpp"
#include "managers/model_builder.hpp"
#include "utils.hpp"
#include "timer.hpp"
#include "debug.hpp"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
using Tyra::Renderer3D;

// ----
// Constructors/Destructors
// ----

Player::Player(Level* pLevel, Renderer* t_renderer,
               ItemRepository* t_itemRepository,
               WorldLightModel* t_worldLightModel)
    : Entity(pLevel, EntityType::Player) {
  this->pLevel = pLevel;
  this->t_renderer = t_renderer;
  this->t_itemRepository = t_itemRepository;
  this->t_worldLightModel = t_worldLightModel;

  loadPlayerTexture();
  loadMesh();
  loadStaticBBox();

  isWalkingAnimationSet = false;
  isBreakingAnimationSet = false;
  isStandStillAnimationSet = false;

  isOnGround = true;
  isFlying = false;
  isBreaking = false;
  collidable = false;

  dynpip.setRenderer(&this->t_renderer->core);
  modelDynpipOptions.antiAliasingEnabled = false;
  modelDynpipOptions.frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;
  modelDynpipOptions.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  modelDynpipOptions.textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;

  // TODO: refactor to handled item, temp stuff...
  // this->handledItem->init(t_renderer);
  stpip.setRenderer(&t_renderer->core);

  // Set render pip
  this->setRenderPip(new PlayerRenderArmPip(this));
}

Player::~Player() {
  delete bbox;
  // delete handledItem;
  delete this->renderPip;

  t_renderer->getTextureRepository().free(playerTexture);
  walkSequence.clear();
  walkSequence.shrink_to_fit();

  breakBlockSequence.clear();
  breakBlockSequence.shrink_to_fit();

  standStillSequence.clear();
  standStillSequence.shrink_to_fit();
}

// ----
// Methods
// ----

void Player::fixedUpdate(const float& fixedDeltaTime, const Vec4& movementDir,
                         Camera* t_camera) {
  // Reset lerp state
  // set new prev to old target
  _prevPosition.set(_targetPosition);

  // Vertical collision and movement
  const float nextYPos = getNextVrticalPosition(fixedDeltaTime);
  updateTerrainHeightAtEntityPosition();
  if (!isFlying) updateYPosition(nextYPos);

  isMoving = movementDir.length();
  // Horizontal collision and movement
  if (isMoving) {
    onMoved();

    // Update player speed
    const float _maxSpeed = isRunning ? runningMaxSpeed : maxSpeed;
    const float _maxAcc = isRunning ? runningAcceleration : acceleration;

    // Accelerate speed until mach max
    if (speed < _maxSpeed) {
      speed += _maxAcc * fixedDeltaTime;
    } else if (speed > _maxSpeed) {
      // Deaccelerate speed to new max
      speed = _maxSpeed;
    }

    Vec4 nextXZPos =
        getNextXZPosition(fixedDeltaTime, movementDir,
                          t_camera->unitCirclePosition.getNormalized());
    updateXZPosition(fixedDeltaTime, nextXZPos);
  } else {
    onStopMoving();

    // Deaccelerate player speed
    if (speed > 0) {
      speed -= acceleration * fixedDeltaTime;
    } else if (speed < 0) {
      speed = 0;
    }
  }

  // TODO: move to player render pip
  if (t_camera->getCamType() != CamType::FirstPerson) {
    mesh.get()->rotation.identity();
    float theta = Tyra::Math::atan2(t_camera->unitCirclePosition.x,
                                    t_camera->unitCirclePosition.z);
    mesh->rotation.rotateY(theta);
  }
}

void Player::update(const float& deltaTime, Camera* t_camera) {
  position.lerp(_prevPosition, _targetPosition, TyraCraft::Timer::stateLerp);
  mesh->getPosition()->set(position);

  updateFovBySpeed();

  renderPip->update(deltaTime, t_camera);
  animate(deltaTime, t_camera->getCamType());
}

void Player::tick() {
  // Update updateStateInWater every 5 ticks
  if (isTicksCounterAt(5)) updateStateInWater();

  // Update base color after updating position
  updateItemColorByCurrentPosition();
}

void Player::render() {
#ifdef DEBUG_MODE
  if (g_debug_menu.showPlayerBoundingBox) {
    t_renderer->renderer3D.utility.drawBBox(getHitBox(), Color(100, 50, 50));
  }
  if (g_debug_menu.enableRenderPlayers == false) return;
#endif  // DEBUG_MODE

  renderPip->render(t_renderer);
}

Vec4 Player::getNextXZPosition(const float& deltaTime, const Vec4& sensibility,
                               const Vec4& camDir) {
  Vec4 direction =
      Vec4((camDir.x * -sensibility.z) + (camDir.z * -sensibility.x), 0.0F,
           (camDir.z * -sensibility.z) + (camDir.x * sensibility.x))
          .getNormalized();

  Vec4 newVelocity = direction * (speed * sensibility.length()) * deltaTime;
  velocity.x = newVelocity.x;
  velocity.z = newVelocity.z;

  if (_isUnderWater || _isOnWater) {
    velocity.x *= IN_WATER_FRICTION;
    velocity.z *= IN_WATER_FRICTION;
  }

  Vec4 result = _targetPosition + Vec4(velocity.x, 0.0f, velocity.z);

  return result.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS) ? result
                                                          : _targetPosition;
}

float Player::getNextVrticalPosition(const float& fixedDeltaTime) {
  if (isFlying) {
    return _targetPosition.y;
  } else {
    velocity.y += GRAVITY.y * fixedDeltaTime;
  }

  if (_isUnderWater) {
    velocity.y *= GRAVITY_UNDER_WATER_FACTOR;
  } else if (_isOnWater) {
    velocity.y *= GRAVITY_ON_WATER_FACTOR;
  }

  return _targetPosition.y + (velocity.y * fixedDeltaTime);
}

const float Player::getHeight() { return Utils::Abs(bbox->getHeight()); }

void Player::resolveOutOfWorldBoundaries() {
  spawnArea.print("Reseting player position to:");
  velocity = Vec4(0.0f, 0.0f, 0.0f);
  setPosition(spawnArea);
};

void Player::onMoved() {
  if (lastTimePlayedWalkSfx > 0.35f) {
    if (isOnWater() || isUnderWater()) {
      playSwimSfx();
    } else if (isOnGround) {
      playWalkSfx(static_cast<Blocks>(terrainHeight.lowerBlockType));
    }

    setWalkingAnimation();
    lastTimePlayedWalkSfx = 0.0F;
  } else {
    lastTimePlayedWalkSfx +=
        TyraCraft::Timer::getInstance()->getFixedDeltaTime();
  }
}

void Player::onStopMoving() { unsetWalkingAnimation(); }

/** Fly in up direction */
void Player::flyUp(const float& deltaTime) {
  const Vec4 upDir = -GRAVITY;
  this->fly(deltaTime, terrainHeight, upDir.getNormalized());
}

/** Fly in down direction */
void Player::flyDown(const float& deltaTime) {
  const Vec4 downDir = GRAVITY;
  this->fly(deltaTime, terrainHeight, downDir.getNormalized());
}

/** Fly in given direction */
void Player::fly(const float& deltaTime,
                 const TerrainHeightModel& terrainHeight,
                 const Vec4& direction) {
  float newYPos = _targetPosition.y + (direction.y * maxSpeed * deltaTime);
  const float playerHeight = Utils::Abs(bbox->getHeight());

  // Is player inside world bbox?
  if (newYPos + playerHeight >= (OVERWORLD_MAX_HEIGH * DOUBLE_BLOCK_SIZE) ||
      newYPos < (OVERWORLD_MIN_HEIGH * DOUBLE_BLOCK_SIZE)) {
    return;
  } else {
    if (newYPos < terrainHeight.minHeight) {
      newYPos = terrainHeight.minHeight;
      this->isOnGround = true;
      this->isFlying = false;
    }

    if (newYPos + playerHeight > terrainHeight.maxHeight) {
      newYPos = terrainHeight.maxHeight - playerHeight - 1.0F;
    }

    _targetPosition.y = newYPos;
  }
}

u8 Player::updateXZPosition(const float& deltaTime, const Vec4& nextPlayerPos,
                            u8 isColliding) {
  const float maxCollidableDistance = _targetPosition.distanceTo(nextPlayerPos);
  const Vec4 positionDiff = nextPlayerPos - _targetPosition;
  const Vec4 direction = positionDiff.getNormalized();

  float shortestDistance = -1.0f;
  float tempHitDistance = -1.0f;
  u8 autoJump = false;

  // Temp custom model expanded by player volocity
  M4x4 model = M4x4::Identity;
  Vec4 deltaScale = Vec4(std::abs(positionDiff.x), std::abs(positionDiff.y),
                         std::abs(positionDiff.z));
  const Vec4 hitBoxSize = getHitBoxSize();
  Vec4 scaleFraction = (deltaScale / hitBoxSize);
  model.scale(scaleFraction + Vec4(1.0F, 1.0F, 1.0F));
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
  Vec4 playerMin, playerMax;
  BBox playerBB = getHitBox();
  playerBB.getMinMax(&playerMin, &playerMax);

  const Vec4 origin = ((playerMax - playerMin) / 2) + playerMin;

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

    // Remove blocks that are lower or higher than player bbox
    if (playerBB.getBottomFace().axisPosition >= max.y ||
        playerBB.getTopFace().axisPosition < min.y)
      continue;

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(playerMin, playerMax, min, max, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      if (tempHitDistance > maxCollidableDistance) continue;

      if (shortestDistance == -1.0f || tempHitDistance < shortestDistance) {
        shortestDistance = tempHitDistance;
        // It's truen the block is lower than player and can be auto jumped
        // e.g. slab
        autoJump = (max.y - playerMin.y) <= BLOCK_SIZE;
      }
    }
  }

  // Will collide somewhere?
  if (shortestDistance > -1.0f) {
    const float timeToHit = shortestDistance / speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        shortestDistance < _prevPosition.distanceTo(nextPlayerPos)) {
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
                 Vec4(nextPlayerPos.x, _targetPosition.y, _targetPosition.z),
                 true) ||
             updateXZPosition(
                 deltaTime,
                 Vec4(_targetPosition.x, _targetPosition.y, nextPlayerPos.z),
                 true);
    }
  }

  // Apply new position;
  _targetPosition.x = nextPlayerPos.x;
  _targetPosition.z = nextPlayerPos.z;
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
  Vec4 playerMin, playerMax;
  BBox playerBB = getHitBox();
  playerBB.getMinMax(&playerMin, &playerMax);

  const Vec4 origin = ((playerMax - playerMin) / 2) + playerMin;

  // Prepate the raycast
  const Ray ray = Ray(origin, direction);

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = static_cast<Entity*>(g_AABBTree->user_data(ni[i]));
    if (!entity->collidable) continue;

    if (playerBB.getBottomFace().axisPosition >= entity->maxCorner.y ||
        playerBB.getTopFace().axisPosition < entity->minCorner.y)
      continue;

    Vec4 tempInflatedMin;
    Vec4 tempInflatedMax;
    Utils::GetMinkowskiSum(playerMin, playerMax, entity->minCorner,
                           entity->maxCorner, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      if (tempHitDistance > maxCollidableDistance) continue;

      if (shortestDistance == -1.0f || tempHitDistance < shortestDistance) {
        shortestDistance = tempHitDistance;
        autoJump = entity->maxCorner.y > playerMin.y &&
                   (entity->maxCorner.y - playerMin.y <= BLOCK_SIZE);
      }
    }
  }

  // Will collide somewhere?
  if (shortestDistance > -1.0f) {
    const float timeToHit = shortestDistance / speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        shortestDistance < _prevPosition.distanceTo(nextPlayerPos)) {
      if (isColliding) {
        return false;
      }

      // Check if can jump
      if (autoJump && isOnGround) {
        jumpQuickly();
        return true;
      }

      // Try to move in separated axis;
      Vec4 moveOnXOnly =
          Vec4(nextPlayerPos.x, _targetPosition.y, _targetPosition.z);
      if (updateXZPosition(deltaTime, moveOnXOnly, true)) return true;

      Vec4 moveOnZOnly =
          Vec4(_targetPosition.x, _targetPosition.y, nextPlayerPos.z);
      if (updateXZPosition(deltaTime, moveOnZOnly, true)) return true;

      return false;
    }
  }
  */
}

/**
 * Inventory controllers
 *
 */

ItemId Player::getSelectedInventoryItemType() {
  return this->inventory[this->selectedInventoryIndex];
}

/**
 * @brief Return selected slot - int between 1 and 9
 *
 */
u8 Player::getSelectedInventorySlot() {
  return this->selectedInventoryIndex + 1;
}

void Player::moveSelectorToTheLeft() {
  selectedInventoryIndex--;
  if (selectedInventoryIndex < 0)
    selectedInventoryIndex = HOT_INVENTORY_SIZE - 1;
  selectedSlotHasChanged = 1;

  const auto currentItemId = inventory[selectedInventoryIndex];
  if (currentItemId == ItemId::empty) {
    this->updateHandledItem();
  } else {
    this->updateHandledItem();
  }
}

void Player::moveSelectorToTheRight() {
  selectedInventoryIndex++;
  if (selectedInventoryIndex > HOT_INVENTORY_SIZE - 1)
    selectedInventoryIndex = 0;
  selectedSlotHasChanged = 1;

  const auto currentItemId = inventory[selectedInventoryIndex];
  if (currentItemId == ItemId::empty) {
    this->updateHandledItem();
  } else {
    this->updateHandledItem();
  }
}

void Player::fillInventoryWithItem(ItemId itemId) {
  for (size_t i = 0; i < HOT_INVENTORY_SIZE; i++) {
    inventory[i] = itemId;
  }
}

void Player::loadMesh() {
  ObjLoaderOptions options;
  options.scale = 15.0F;
  options.flipUVs = true;
  options.animation.count = 10;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("models/player/player.obj"), options);
  data.get()->loadNormals = false;

  this->mesh = std::make_unique<DynamicMesh>(data.get());

  this->mesh->rotation.identity();
  this->mesh->scale.identity();
  this->mesh->scale.scaleX(0.85F);

  auto& materials = this->mesh.get()->materials;
  for (size_t i = 0; i < materials.size(); i++)
    playerTexture->addLink(materials[i]->id);

  this->mesh->animation.loop = true;
  this->mesh->animation.setSequence(standStillSequence);
  // Speed is treated as frames-per-second; actual step will be multiplied by
  // deltaTime in animate()
  this->mesh->animation.speed = baseAnimationSpeed;
}

void Player::loadStaticBBox() {
  const Vec4 hitBoxSize = getHitBoxSize();
  Vec4 minCorner = Vec4(-hitBoxSize.x, 0, -hitBoxSize.z);
  Vec4 maxCorner = Vec4(hitBoxSize.x, hitBoxSize.y, hitBoxSize.z);

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

void Player::playWalkSfx(const Blocks& blockType) {
  SfxBlockModel* blockSfxModel =
      BlockManager::getInstance()->getStepSoundByBlockType(blockType);
  if (blockSfxModel) {
    SoundManager* pSoundManager = SoundManager::getInstance();
    const int ch = pSoundManager->getAvailableChannel();
    SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);

    auto config = SfxConfig::getStepSoundConfig(blockType);
    sound->_sound->pitch = config->_pitch;
    pSoundManager->setSfxVolume(config->_volume, ch);
    pSoundManager->playSfx(sound, ch);
    delete config;
  }
}

// Pitch values from
// https://minecraft.fandom.com/wiki/Water#cite_note-bugMC-177092-7
void Player::playSwimSfx() {
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

void Player::playSplashSfx() {
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

void Player::toggleFlying() {
  isFlying = !isFlying;
  if (isFlying) isOnGround = false;
}

void Player::setRunning(bool _isRunning) {
  if (isRunning != _isRunning) {
    isRunning = _isRunning;
  }
}

void Player::selectNextItem() {
  int currentItemId = (int)inventory[selectedInventoryIndex];
  ItemId nextItem;

  if ((currentItemId + 1) >= (int)ItemId::total_of_items) {
    nextItem = ItemId::empty;
  } else {
    nextItem = static_cast<ItemId>(currentItemId + 1);
  }

  inventory[selectedInventoryIndex] = nextItem;
  this->inventoryHasChanged = true;

  if (nextItem == ItemId::empty) {
    this->updateHandledItem();
  } else {
    this->updateHandledItem();
  }
}

void Player::selectPreviousItem() {
  int currentItemId = (int)inventory[selectedInventoryIndex];
  ItemId previousItem;

  if ((currentItemId - 1) < 0) {
    previousItem = static_cast<ItemId>((u8)ItemId::total_of_items - 1);
  } else {
    previousItem = static_cast<ItemId>(currentItemId - 1);
  }

  inventory[selectedInventoryIndex] = previousItem;
  this->inventoryHasChanged = true;

  if (previousItem == ItemId::empty) {
    this->updateHandledItem();
  } else {
    this->updateHandledItem();
  }
}

void Player::jump() {
  velocity += lift;
  isOnGround = false;
}

void Player::jumpQuickly() {
  velocity += lift * 0.75f;
  isOnGround = false;
}

void Player::swim() {
  if (_isUnderWater) {
    velocity += (lift * 0.25F);
  } else if (_isOnWater) {
    velocity += (lift * 0.25F);
    _isOnWater = false;
  }

  if (velocity.y > lift.y) velocity.y = lift.y;

  isOnGround = false;
}

void Player::setRenderPip(PlayerRenderPip* pipToSet) {
  delete this->renderPip;
  this->renderPip = pipToSet;
}

void Player::setArmBreakingAnimation() {
  if (isBreakingAnimationSet) return;

  mesh->animation.speed = baseAnimationSpeed * 3;
  mesh->animation.setSequence(breakBlockSequence);

  isBreakingAnimationSet = true;
  isBreaking = true;
}

void Player::unsetArmBreakingAnimation() {
  if (!isBreakingAnimationSet) return;

  isBreaking = false;
  isBreakingAnimationSet = false;
  mesh->animation.setSequence(standStillSequence);
}

void Player::setWalkingAnimation() {
  const float _speed = (speed / runningMaxSpeed) * 4;
  this->mesh->animation.speed = baseAnimationSpeed * _speed;

  if (!isWalkingAnimationSet) {
    this->mesh->animation.setSequence(walkSequence);
    isWalkingAnimationSet = true;
  }
}

void Player::unsetWalkingAnimation() {
  if (!isWalkingAnimationSet) return;

  isWalkingAnimationSet = false;
  mesh->animation.setSequence(standStillSequence);
}

void Player::playPutBlockAnimation() { isPuting = true; }

void Player::stopPutBlockAnimation() { isPuting = false; }

void Player::animate(const float& deltaTime, CamType camType) {
  if (camType == CamType::ThirdPerson) {
    // Make animation step time-based: scale current animation speed by delta
    // time for this frame
    const float originalSpeed = this->mesh->animation.speed;
    this->mesh->animation.speed = originalSpeed * deltaTime;
    this->mesh->update();
    // Restore speed so game logic can continue to treat it as frames-per-second
    this->mesh->animation.speed = originalSpeed;
  }
}

void Player::shiftItemToInventory(const ItemId& itemToShift) {
  for (size_t i = HOT_INVENTORY_SIZE - 1; i > 0; i--) {
    inventory[i] = inventory[i - 1];
  }

  inventory[0] = itemToShift;
  inventoryHasChanged = true;
}

void Player::setItemToInventory(const ItemId& itemToShift) {
  inventory[selectedInventoryIndex] = itemToShift;
  inventoryHasChanged = true;
}

void Player::loadPlayerTexture() {
  const auto skinPath =
      std::string("textures/skin/").append(g_settings.skin).append(".png");
  playerTexture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd(skinPath.c_str()));
}

void Player::updateStateInWater() {
  Vec4 min, mid, max, top, bottom;
  BBox bbox = getHitBox();
  bbox.getMinMax(&min, &max);
  mid = ((max - min) / 2) + min;

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

  // printf("isOnWater: %i | isUnderWater: %i\n", _isOnWater, _isUnderWater);
}

bool Player::isOnWater() { return _isOnWater; }

bool Player::isUnderWater() { return _isUnderWater; }

void Player::setRenderArmPip() { setRenderPip(new PlayerRenderArmPip(this)); }

void Player::setRenderBodyPip() { setRenderPip(new PlayerRenderBodyPip(this)); }

void Player::updateFovBySpeed() {
  const float _speed = speed < maxSpeed ? maxSpeed : speed;
  const float _minSpeed = maxSpeed;
  const float _maxSpeed = runningMaxSpeed;
  float _fovByLerp;

  if (isFlying) {
    _fovByLerp = Utils::reRangeScale(_minFovFlaying, _maxFovFlaying, _minSpeed,
                                     _maxSpeed, _speed);
  } else {
    _fovByLerp =
        Utils::reRangeScale(_minFov, _maxFov, _minSpeed, _maxSpeed, _speed);
  }
  t_renderer->core.renderer3D.setFov(_fovByLerp);
}

void Player::updateItemColorByCurrentPosition() {
  const Vec4 pos = (position / DOUBLE_BLOCK_SIZE);
  const Vec4 offset = Vec4(std::floor(pos.x + 0.5f), std::floor(pos.y + 1),
                           std::floor(pos.z + 0.5f));

  if (pLevel->BoundCheckMap(offset.x, offset.y, offset.z)) {
    const int lvl = pLevel->GetLightDataFromMap(offset.x, offset.y, offset.z);
    const float s_lvl = static_cast<float>((lvl >> 4) & 0xF) *
                        t_worldLightModel->sunLightIntensity;
    const float b_lvl = static_cast<float>(lvl & 0x0F);
    const auto maxLevel = std::max({s_lvl, b_lvl, 3.0f});
    const float color = 128.0f * (maxLevel / 15.0f);

    _baseColorAtPlayerPos.set(color, color, color);
  }
};
