#include "entities/player/player.hpp"
#include "entities/level.hpp"

using Tyra::Renderer3D;

// ----
// Constructors/Destructors
// ----

Player::Player(Renderer* t_renderer, SoundManager* t_soundManager,
               BlockManager* t_blockManager) {
  this->t_renderer = t_renderer;
  this->t_blockManager = t_blockManager;
  this->t_soundManager = t_soundManager;

  loadPlayerTexture();
  loadMesh();
  loadArmMesh();
  calcStaticBBox();

  isWalkingAnimationSet = false;
  isBreakingAnimationSet = false;
  isStandStillAnimationSet = false;

  isOnGround = true;
  isFlying = false;
  isBreaking = false;

  dynpip.setRenderer(&this->t_renderer->core);
  modelDynpipOptions.antiAliasingEnabled = false;
  modelDynpipOptions.frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;
  modelDynpipOptions.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  modelDynpipOptions.textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;

  // TODO: refactor to handled item, temp stuff...
  {
    this->handledItem->init(t_renderer);
    stpip.setRenderer(&t_renderer->core);
  }

  // Set render pip
  this->setRenderPip(new PlayerRenderArmPip(this));
}

Player::~Player() {
  currentBottomBlock = nullptr;
  currentUpperBlock = nullptr;

  delete hitBox;
  delete handledItem;
  delete this->renderPip;
  t_renderer->getTextureRepository().free(playerTexture);
  walkSequence.clear();
  walkSequence.shrink_to_fit();

  breakBlockSequence.clear();
  breakBlockSequence.shrink_to_fit();

  standStillSequence.clear();
  standStillSequence.shrink_to_fit();

  armStandStillSequence.clear();
  armStandStillSequence.shrink_to_fit();

  armWalkingSequence.clear();
  armWalkingSequence.shrink_to_fit();

  armHitingSequence.clear();
  armHitingSequence.shrink_to_fit();
}

// ----
// Methods
// ----

void Player::update(const float& deltaTime, const Vec4& movementDir,
                    Camera* t_camera, const std::vector<Chunck*>& loadedChunks,
                    TerrainHeightModel* terrainHeight, LevelMap* t_terrain) {
  updateStateInWater(t_terrain);
  isMoving = movementDir.length() > 0;

  if (isMoving) {
    Vec4 nextPlayerPos = getNextPosition(
        deltaTime, movementDir, t_camera->unitCirclePosition.getNormalized());

    if (nextPlayerPos.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS)) {
      const bool hasChangedPosition =
          updatePosition(loadedChunks, deltaTime, nextPlayerPos);

      if (hasChangedPosition) {
        if (lastTimePlayedWalkSfx > 0.35) {
          if (isOnWater() || isUnderWater()) {
            playSwimSfx();
          } else if (isOnGround && currentBottomBlock) {
            playWalkSfx(currentBottomBlock->type);
          }

          setWalkingAnimation();
          lastTimePlayedWalkSfx = 0.0F;
        } else {
          lastTimePlayedWalkSfx += deltaTime;
        }
      }
    }
  } else {
    // Deaccelerate player speed
    if (speed > 0) {
      speed -= acceleration * deltaTime;
    } else if (speed < 0) {
      speed = 0;
      unsetWalkingAnimation();
    }
  }

  updateFovBySpeed();

  if (t_camera->getCamType() != CamType::FirstPerson) {
    mesh.get()->rotation.identity();
    float theta = Tyra::Math::atan2(t_camera->unitCirclePosition.x,
                                    t_camera->unitCirclePosition.z);
    mesh->rotation.rotateY(theta);
  }

  if (!isFlying) updateGravity(deltaTime, terrainHeight);

  animate(t_camera->getCamType());

  // this->handledItem->mesh->translation.identity();
  // this->handledItem->mesh->translation.operator*=(this->mesh->translation);
}

void Player::render() {
  renderPip->render(t_renderer);

  // // Debug content
  // if (currentUpperBlock) {
  //   t_renderer->renderer3D.utility.drawBBox(*currentUpperBlock->bbox,
  //                                           Color(200, 0, 0));
  // }
  // if (currentBottomBlock) {
  //   t_renderer->renderer3D.utility.drawBBox(*currentBottomBlock->bbox,
  //                                           Color(0, 200, 0));
  // }

  // t_renderer->renderer3D.utility.drawBBox(getHitBox(), Color(200, 200, 0));
}

Vec4 Player::getNextPosition(const float& deltaTime, const Vec4& sensibility,
                             const Vec4& camDir) {
  const float _maxSpeed = isRunning ? runningMaxSpeed : maxSpeed;
  const float _maxAcc = isRunning ? runningAcceleration : acceleration;

  // Accelerate speed until mach max
  if (speed < _maxSpeed) {
    speed += _maxAcc * deltaTime;
  } else if (speed > _maxSpeed) {
    // Deaccelerate speed to new max
    speed -= _maxAcc * deltaTime;
    if (speed < _maxSpeed) speed = _maxSpeed;
  }

  Vec4 direction =
      Vec4((camDir.x * -sensibility.z) + (camDir.z * -sensibility.x), 0.0F,
           (camDir.z * -sensibility.z) + (camDir.x * sensibility.x))
          .getNormalized();

  Vec4 result = direction * (speed * sensibility.length() * deltaTime);

  if (_isUnderWater || _isOnWater) {
    result *= IN_WATER_FRICTION;
  }

  return result + *mesh->getPosition();
}

/** Update player position by gravity and update index of current block */
void Player::updateGravity(const float& deltaTime,
                           TerrainHeightModel* terrainHeight) {
  // Accelerate the velocity: velocity += gravConst * deltaTime
  velocity += Vec4(velocity.x, GRAVITY.y * deltaTime, velocity.z);

  if (_isUnderWater) {
    velocity.y *= GRAVITY_UNDER_WATER_FACTOR;
  } else if (_isOnWater) {
    velocity.y *= GRAVITY_ON_WATER_FACTOR;
  }

  // Increase the position by velocity
  Vec4 newPosition = *mesh->getPosition() + (velocity * deltaTime);

  const float worldMinHeight = OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE;
  const float worldMaxHeight = OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE;
  if (newPosition.y + hitBox->getHeight() > worldMaxHeight ||
      newPosition.y < worldMinHeight) {
    // Maybe has died, teleport to spaw area
    TYRA_LOG("\nReseting player position to:\n");
    mesh->getPosition()->set(spawnArea);
    velocity = Vec4(0.0f, 0.0f, 0.0f);
    return;
  }

  const float playerHeight = std::abs(hitBox->getHeight());
  const float heightLimit = terrainHeight->maxHeight - playerHeight;

  if (newPosition.y < terrainHeight->minHeight) {
    newPosition.y = terrainHeight->minHeight;
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

/** Fly in up direction */
void Player::flyUp(const float& deltaTime,
                   const TerrainHeightModel& terrainHeight) {
  const Vec4 upDir = -GRAVITY * 0.35F;
  this->fly(deltaTime, terrainHeight, upDir);
}

/** Fly in down direction */
void Player::flyDown(const float& deltaTime,
                     const TerrainHeightModel& terrainHeight) {
  const Vec4 downDir = GRAVITY * 0.35F;
  this->fly(deltaTime, terrainHeight, downDir);
}

/** Fly in given direction */
void Player::fly(const float& deltaTime,
                 const TerrainHeightModel& terrainHeight,
                 const Vec4& direction) {
  Vec4 newYPosition = *mesh->getPosition() + (direction * deltaTime);
  const float playerHeight = std::abs(hitBox->getHeight());

  // Is player inside world bbox?
  if (newYPosition.y + playerHeight >=
          (OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE) ||
      newYPosition.y < (OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE)) {
    return;
  } else {
    if (newYPosition.y < terrainHeight.minHeight) {
      newYPosition.y = terrainHeight.minHeight;
      this->isOnGround = true;
      this->isFlying = false;
    }

    if (newYPosition.y + playerHeight > terrainHeight.maxHeight) {
      newYPosition.y = terrainHeight.maxHeight - playerHeight - 1.0F;
    }

    mesh->getPosition()->set(newYPosition);
  }
}

u8 Player::updatePosition(const std::vector<Chunck*>& loadedChunks,
                          const float& deltaTime, const Vec4& nextPlayerPos,
                          u8 isColliding) {
  Vec4 currentPlayerPos = *this->mesh->getPosition();
  Vec4 playerMin;
  Vec4 playerMax;
  BBox playerBB = getHitBox();
  playerBB.getMinMax(&playerMin, &playerMax);

  // Set ray props
  Vec4 rayOrigin = ((playerMax - playerMin) / 2) + playerMin;
  Vec4 rayDir = nextPlayerPos - currentPlayerPos;
  rayDir.normalize();
  const Ray ray = Ray(rayOrigin, rayDir);

  float finalHitDistance = -1.0f;
  float tempHitDistance = -1.0f;
  const float maxCollidableDistance =
      currentPlayerPos.distanceTo(nextPlayerPos);

  for (size_t chunkIndex = 0; chunkIndex < loadedChunks.size(); chunkIndex++) {
    for (size_t i = 0; i < loadedChunks[chunkIndex]->blocks.size(); i++) {
      // Broad phase

      auto block = loadedChunks[chunkIndex]->blocks[i];

      if (
          // Prevent colliding to water horizontally
          !block->isCollidable ||

          // is vertically out of range?
          (playerBB.getBottomFace().axisPosition >= block->maxCorner.y ||
           playerBB.getTopFace().axisPosition < block->minCorner.y ||
           currentPlayerPos.distanceTo(block->position) >
               DUBLE_BLOCK_SIZE * 2)) {
        continue;
      };

      Vec4 tempInflatedMin;
      Vec4 tempInflatedMax;
      Utils::GetMinkowskiSum(playerMin, playerMax, block->minCorner,
                             block->maxCorner, &tempInflatedMin,
                             &tempInflatedMax);

      if (ray.intersectBox(tempInflatedMin, tempInflatedMax,
                           &tempHitDistance)) {
        // Is horizontally collidable?
        if (tempHitDistance > maxCollidableDistance) continue;

        if (finalHitDistance == -1.0f || tempHitDistance < finalHitDistance) {
          finalHitDistance = tempHitDistance;
        }
      }
    }
  }

  // Will collide somewhere;
  if (finalHitDistance > -1.0f) {
    const float timeToHit = finalHitDistance / this->speed;

    // Will collide this frame;
    if (timeToHit < deltaTime ||
        finalHitDistance <
            this->mesh->getPosition()->distanceTo(nextPlayerPos)) {
      if (isColliding) return false;

      // Try to move in separated axis;
      Vec4 moveOnXOnly =
          Vec4(nextPlayerPos.x, currentPlayerPos.y, currentPlayerPos.z);
      if (updatePosition(loadedChunks, deltaTime, moveOnXOnly, 1)) return true;

      Vec4 moveOnZOnly =
          Vec4(currentPlayerPos.x, nextPlayerPos.y, currentPlayerPos.z);
      if (updatePosition(loadedChunks, deltaTime, moveOnZOnly, 1)) return true;

      return false;
    }
  }

  // Apply new position;
  mesh->getPosition()->set(nextPlayerPos);
  return true;
}

TerrainHeightModel Player::getTerrainHeightAtPosition(
    const std::vector<Chunck*>& loadedChunks) {
  TerrainHeightModel model;
  BBox playerBB = this->getHitBox();
  Vec4 minPlayer, maxPlayer;
  playerBB.getMinMax(&minPlayer, &maxPlayer);

  currentBottomBlock = nullptr;
  currentUpperBlock = nullptr;

  for (size_t chunkIndex = 0; chunkIndex < loadedChunks.size(); chunkIndex++) {
    for (size_t i = 0; i < loadedChunks[chunkIndex]->blocks.size(); i++) {
      Block* block = loadedChunks[chunkIndex]->blocks[i];

      if (
          // Is collidable
          block->isCollidable &&

          // is under or above block
          minPlayer.x <= block->maxCorner.x &&
          maxPlayer.x >= block->minCorner.x &&
          minPlayer.z <= block->maxCorner.z &&
          maxPlayer.z >= block->minCorner.z) {
        const float underBlockHeight = block->maxCorner.y;
        if (minPlayer.y >= underBlockHeight &&
            underBlockHeight > model.minHeight) {
          model.minHeight = underBlockHeight;
          currentBottomBlock = block;
        }

        const float overBlockHeight = block->minCorner.y;
        if (maxPlayer.y < overBlockHeight &&
            overBlockHeight < model.maxHeight) {
          model.maxHeight = overBlockHeight;
          currentUpperBlock = block;
        }
      }
    }
  }

  return model;
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
}

void Player::moveSelectorToTheRight() {
  selectedInventoryIndex++;
  if (selectedInventoryIndex > HOT_INVENTORY_SIZE - 1)
    selectedInventoryIndex = 0;
  selectedSlotHasChanged = 1;
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
  // this->mesh->rotation.rotateY(-3.14F);
  // this->mesh->rotation.rotateY(Tyra::Math::ANG2RAD * -90);
  this->mesh->scale.identity();

  auto& materials = this->mesh.get()->materials;
  for (size_t i = 0; i < materials.size(); i++)
    playerTexture->addLink(materials[i]->id);

  this->mesh->animation.loop = true;
  this->mesh->animation.setSequence(standStillSequence);
  this->mesh->animation.speed = 0.08F;
}

void Player::loadArmMesh() {
  ObjLoaderOptions options;
  options.scale = 15.0F;
  options.flipUVs = true;
  options.animation.count = 21;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("models/player_arm/arm.obj"), options);
  data.get()->loadNormals = false;

  this->armMesh = std::make_unique<DynamicMesh>(data.get());

  this->armMesh->scale.identity();
  this->armMesh->scale.scaleX(0.85F);
  this->armMesh->scale.scaleZ(1.15F);

  this->armMesh->translation.identity();
  this->armMesh->translation.translateZ(-13.0F);
  this->armMesh->translation.translateY(-8.5F);
  this->armMesh->translation.translateX(3.5F);

  this->armMesh->rotation.identity();
  this->armMesh->rotation.rotateY(-3.24);
  this->armMesh->rotation.rotateX(0.35);

  auto& materials = this->armMesh.get()->materials;
  for (size_t i = 0; i < materials.size(); i++)
    playerTexture->addLink(materials[i]->id);

  this->armMesh->animation.loop = true;
  this->armMesh->animation.setSequence(armStandStillSequence);
  this->armMesh->animation.speed = 0.08F;

  armDynpipOptions.antiAliasingEnabled = false;
  armDynpipOptions.frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;
  armDynpipOptions.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  armDynpipOptions.textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;
  armDynpipOptions.transformationType =
      Tyra::PipelineTransformationType::TyraMP;
}

void Player::calcStaticBBox() {
  const float width = (DUBLE_BLOCK_SIZE * 0.4F) / 2;
  const float depth = (DUBLE_BLOCK_SIZE * 0.4F) / 2;
  const float height = DUBLE_BLOCK_SIZE * 1.8F;

  Vec4 minCorner = Vec4(-width, 0, -depth);
  Vec4 maxCorner = Vec4(width, height, depth);

  const u32 count = 8;
  Vec4 vertices[count] = {Vec4(minCorner),
                          Vec4(maxCorner.x, minCorner.y, minCorner.z),
                          Vec4(minCorner.x, maxCorner.y, minCorner.z),
                          Vec4(minCorner.x, minCorner.y, maxCorner.z),
                          Vec4(maxCorner),
                          Vec4(minCorner.x, maxCorner.y, maxCorner.z),
                          Vec4(maxCorner.x, minCorner.y, maxCorner.z),
                          Vec4(maxCorner.x, maxCorner.y, minCorner.z)};

  this->hitBox = new BBox(vertices, count);

  // Hitbox Debug stuff
  // {
  //   TYRA_LOG("==================");
  //   TYRA_LOG("DUBLE_BLOCK_SIZE -> ",
  //   std::to_string(DUBLE_BLOCK_SIZE).c_str()); TYRA_LOG("minCorner");
  //   minCorner.print();
  //   TYRA_LOG("maxCorner");
  //   maxCorner.print();
  //   TYRA_LOG("Player->StaticBBox");
  //   this->hitBox->print();
  //   TYRA_LOG("==================");
  // }
}

void Player::playWalkSfx(const Blocks& blockType) {
  SfxBlockModel* blockSfxModel =
      this->t_blockManager->getStepSoundByBlockType(blockType);
  if (blockSfxModel) {
    const int ch = t_soundManager->getAvailableChannel();
    SfxLibrarySound* sound = t_soundManager->getSound(blockSfxModel);
    auto config = SfxConfig::getStepSoundConfig(blockType);
    sound->_sound->pitch = config->_pitch;
    t_soundManager->setSfxVolume(config->_volume, ch);
    t_soundManager->playSfx(sound, ch);
    delete config;
  }
}

// Pitch values from
// https://minecraft.fandom.com/wiki/Water#cite_note-bugMC-177092-7
void Player::playSwimSfx() {
  const int ch = t_soundManager->getAvailableChannel();
  auto randSwimSfx = static_cast<SoundFX>(Tyra::Math::randomi(
      static_cast<u8>(SoundFX::Swim1), static_cast<u8>(SoundFX::Swim4)));

  const u8 randPich = Tyra::Math::randomi(60, 140);
  const u8 volume = 30;

  SfxLibrarySound* sound =
      t_soundManager->getSound(SoundFxCategory::Liquid, randSwimSfx);
  sound->_sound->pitch = randPich;
  t_soundManager->setSfxVolume(volume, ch);
  t_soundManager->playSfx(sound, ch);
}

void Player::playSplashSfx() {
  const int ch = t_soundManager->getAvailableChannel();
  auto randSwimSfx = static_cast<SoundFX>(Tyra::Math::randomi(
      static_cast<u8>(SoundFX::Splash), static_cast<u8>(SoundFX::Splash2)));

  const u8 randPich = Tyra::Math::randomi(60, 140);
  const u8 volume = 60;

  SfxLibrarySound* sound =
      t_soundManager->getSound(SoundFxCategory::Liquid, randSwimSfx);
  sound->_sound->pitch = randPich;
  t_soundManager->setSfxVolume(volume, ch);
  t_soundManager->playSfx(sound, ch);
}

void Player::toggleFlying() {
  this->isFlying = !this->isFlying;
  if (this->isFlying) {
    this->isOnGround = false;
  } else {
  }
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
}

void Player::jump() {
  velocity += lift;
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

  if (isHandFree()) {
    armMesh->animation.speed = baseAnimationSpeed * 6;
    armMesh->animation.setSequence(armHitingSequence);
  }

  isBreakingAnimationSet = true;
  isBreaking = true;
}

void Player::unsetArmBreakingAnimation() {
  if (!isBreakingAnimationSet) return;

  isBreaking = false;
  isBreakingAnimationSet = false;
  mesh->animation.setSequence(standStillSequence);
  armMesh->animation.setSequence(armStandStillSequence);
}

void Player::setWalkingAnimation() {
  if (isWalkingAnimationSet) return;

  this->mesh->animation.speed = baseAnimationSpeed * speed / 10;
  this->mesh->animation.setSequence(walkSequence);

  if (isHandFree()) {
    this->armMesh->animation.speed = baseAnimationSpeed * 3;
    this->armMesh->animation.setSequence(armWalkingSequence);
  }

  isWalkingAnimationSet = true;
}

void Player::unsetWalkingAnimation() {
  if (!isWalkingAnimationSet) return;

  isWalkingAnimationSet = false;
  armMesh->animation.setSequence(armStandStillSequence);
  mesh->animation.setSequence(standStillSequence);
}

void Player::animate(CamType camType) {
  if (camType == CamType::FirstPerson && isHandFree()) {
    this->armMesh->update();
  } else {
    this->mesh->update();
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
  playerTexture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/entity/player/steve.png"));
}

void Player::updateStateInWater(LevelMap* terrain) {
  Vec4 min, mid, max, top, bottom;
  BBox bbox = getHitBox();
  bbox.getMinMax(&min, &max);
  mid = ((max - min) / 2) + min;

  bottom.set(mid.x, min.y, mid.z);
  top.set(mid.x, max.y + 6.0F, mid.z);

  auto blockBottom =
      static_cast<Blocks>(getBlockByWorldPosition(terrain, &bottom));
  auto blockTop = static_cast<Blocks>(getBlockByWorldPosition(terrain, &top));

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
    _fovByLerp = Utils::reRangeScale(_minFovFlaying, _maxFovFlaying, _minSpeed,
                                     _maxSpeed, _speed);
  }
  t_renderer->core.renderer3D.setFov(_fovByLerp);
}