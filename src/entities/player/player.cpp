#include "entities/player/player.hpp"

using Tyra::Renderer3D;

// ----
// Constructors/Destructors
// ----

Player::Player(Renderer* t_renderer, SoundManager* t_soundManager,
               BlockManager* t_blockManager) {
  this->t_renderer = t_renderer;
  this->t_blockManager = t_blockManager;
  this->t_soundManager = t_soundManager;

  this->loadMesh();
  this->loadArmMesh();
  this->calcStaticBBox();

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
  this->setRenderPip(new PlayerFirstPersonRenderPip(this));
}

Player::~Player() {
  currentBlock = nullptr;
  delete hitBox;
  delete handledItem;

  this->t_renderer->getTextureRepository().freeByMesh(mesh.get());
  this->t_renderer->getTextureRepository().freeByMesh(armMesh.get());

  walkSequence.clear();
  breakBlockSequence.clear();
  standStillSequence.clear();
  armStandStillSequence.clear();
  armWalkingSequence.clear();
  armHitingSequence.clear();
}

// ----
// Methods
// ----

void Player::update(const float& deltaTime, Pad& t_pad, Camera& t_camera,
                    std::vector<Chunck*> loadedChunks) {
  this->handleInputCommands(t_pad);

  const Vec4 nextPlayerPos = getNextPosition(deltaTime, t_pad, t_camera);

  if (nextPlayerPos.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS)) {
    const u8 hasMoved =
        this->updatePosition(loadedChunks, deltaTime, nextPlayerPos);

    const u8 canPlayWalkSound = t_pad.getLeftJoyPad().isMoved && hasMoved &&
                                this->isOnGround &&
                                this->currentBlock != nullptr;
    if (canPlayWalkSound) this->playWalkSfx(this->currentBlock->type);
  }

  TerrainHeightModel terrainHeight =
      this->getTerrainHeightAtPosition(loadedChunks);

  if (this->isFlying) {
    if (t_pad.getPressed().DpadUp) {
      this->flyUp(deltaTime, terrainHeight);
    } else if (t_pad.getPressed().DpadDown) {
      this->flyDown(deltaTime, terrainHeight);
    }
  } else {
    this->updateGravity(deltaTime, terrainHeight);
  }

  // this->handledItem->mesh->translation.identity();
  // this->handledItem->mesh->translation.operator*=(this->mesh->translation);
}

void Player::render() {
  this->renderPip->render(this->t_renderer);

  // auto& utilityTools = this->t_renderer->renderer3D.utility;

  // Draw Player bbox
  // { utilityTools.drawBBox(getHitBox()); }

  // if (this->getSelectedInventoryItemType() == ItemId::wooden_axe) {
  //   // TODO: refactor to handledItem structure
  //   this->t_renderer->renderer3D.usePipeline(stpip);
  //   this->stpip.render(this->handledItem->mesh.get(),
  //                      this->handledItem->options);

  //   utilityTools.drawBBox(
  //       this->handledItem->mesh.get()->frame->bbox->getTransformed(
  //           this->handledItem->mesh.get()->getModelMatrix()));
  // }
}

void Player::handleInputCommands(const Pad& t_pad) {
  const PadButtons& clicked = t_pad.getClicked();
  const PadButtons& pressed = t_pad.getPressed();

  if (clicked.L1) this->moveSelectorToTheLeft();
  if (clicked.R1) this->moveSelectorToTheRight();

  if (pressed.L2) {
    this->isBreaking = true;
    if (!isBreakingAnimationSet) {
      // FIXME: enable on thrid person only
      // this->mesh->animation.speed = baseAnimationSpeed * 3;
      // this->mesh->animation.setSequence(breakBlockSequence);

      this->armMesh->animation.speed = baseAnimationSpeed * 6;
      this->armMesh->animation.setSequence(armHitingSequence);
      isBreakingAnimationSet = true;
    }
  } else {
    this->isBreaking = false;
    isBreakingAnimationSet = false;
  }

  if (pressed.Cross && this->isOnGround) {
    this->jump();
  }

  // TODO: refactore to ingame invetory menu
  if (isOnGround) {
    if (clicked.DpadUp) this->selectNextItem();
    if (clicked.DpadDown) this->selectPreviousItem();
  }

  // FIXME: Player mesh rotation based on camera direction
  // if (t_pad.getRightJoyPad().h >= 200) {
  //   this->mesh->rotation.rotateY(-0.08F);
  // } else if (t_pad.getRightJoyPad().h <= 100) {
  //   this->mesh->rotation.rotateY(0.08F);
  // }

  if (t_pad.getLeftJoyPad().isMoved && isOnGround) {
    if (!isWalkingAnimationSet) {
      // TODO: enable on third person only
      // this->mesh->animation.speed = baseAnimationSpeed;
      // this->mesh->animation.setSequence(walkSequence);

      this->armMesh->animation.speed = baseAnimationSpeed * 3;
      this->armMesh->animation.setSequence(armWalkingSequence);
      isWalkingAnimationSet = true;
    }
  } else {
    isWalkingAnimationSet = false;
  }

  // If is not animating walking or breaking, set default (standstill);
  const bool isNotAnimating =
      (!isBreakingAnimationSet && !isWalkingAnimationSet);
  if (isNotAnimating) {
    if (!isStandStillAnimationSet) {
      this->mesh->animation.setSequence(standStillSequence);
      this->armMesh->animation.setSequence(armStandStillSequence);
      this->isStandStillAnimationSet = true;
    }
  } else {
    this->isStandStillAnimationSet = false;
  }

  this->mesh->update();
  this->armMesh->update();
}

Vec4 Player::getNextPosition(const float& deltaTime, Pad& t_pad,
                             const Camera& t_camera) {
  if (t_pad.getLeftJoyPad().isCentered) return (*mesh->getPosition());

  Vec4 camDir = t_camera.unitCirclePosition.getNormalized();
  Vec4 sensibility = Vec4((t_pad.getLeftJoyPad().h - 128.0F) / 128.0F, 0.0F,
                          (t_pad.getLeftJoyPad().v - 128.0F) / 128.0F);
  Vec4 result =
      Vec4((camDir.x * -sensibility.z) + (camDir.z * -sensibility.x), 0.0F,
           (camDir.z * -sensibility.z) + (camDir.x * sensibility.x));
  result.normalize();
  result *=
      (this->speed * sensibility.length() * std::min(deltaTime, MAX_FRAME_MS));
  return result + *mesh->getPosition();
}

/** Update player position by gravity and update index of current block */
void Player::updateGravity(const float& deltaTime,
                           const TerrainHeightModel& terrainHeight) {
  const float dTime = std::min(deltaTime, MAX_FRAME_MS);

  // Accelerate the velocity: velocity += gravConst * deltaTime
  Vec4 acceleration = GRAVITY * dTime;
  this->velocity += acceleration;

  // Increase the position by velocity
  Vec4 newYPosition = *mesh->getPosition() - (this->velocity * dTime);

  if (newYPosition.y + hitBox->getHeight() >=
          OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE ||
      newYPosition.y < OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE) {
    // Maybe has died, teleport to spaw area
    TYRA_LOG("\nReseting player position to:\n");
    this->mesh->getPosition()->set(this->spawnArea);

    this->velocity = Vec4(0.0f, 0.0f, 0.0f);
    return;
  }

  if (newYPosition.y < terrainHeight.minHeight) {
    newYPosition.y = terrainHeight.minHeight;
    this->velocity = Vec4(0.0f, 0.0f, 0.0f);
    this->isOnGround = true;
  } else if (newYPosition.y >
             terrainHeight.maxHeight + this->hitBox->getHeight()) {
    newYPosition.y = terrainHeight.maxHeight + this->hitBox->getHeight();
    this->velocity = -this->velocity;
    this->isOnGround = false;
  }

  // Finally updates gravity after checks
  mesh->getPosition()->set(newYPosition);
}

/** Fly in up direction */
void Player::flyUp(const float& deltaTime,
                   const TerrainHeightModel& terrainHeight) {
  const Vec4 upDir = GRAVITY * -10.0F * deltaTime;
  this->fly(std::min(deltaTime, MAX_FRAME_MS), terrainHeight, upDir);
}

/** Fly in down direction */
void Player::flyDown(const float& deltaTime,
                     const TerrainHeightModel& terrainHeight) {
  const Vec4 downDir = GRAVITY * 10.0F * deltaTime;
  this->fly(std::min(deltaTime, MAX_FRAME_MS), terrainHeight, downDir);
}

/** Fly in given direction */
void Player::fly(const float& deltaTime,
                 const TerrainHeightModel& terrainHeight,
                 const Vec4& direction) {
  Vec4 newYPosition = *mesh->getPosition() - (direction * deltaTime);

  // Is player inside world bbox?
  if (newYPosition.y + hitBox->getHeight() >=
          OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE ||
      newYPosition.y < OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE) {
    return;
  }

  if (newYPosition.y < terrainHeight.minHeight) {
    newYPosition.y = terrainHeight.minHeight;
    this->isOnGround = true;
  } else if (newYPosition.y >
             terrainHeight.maxHeight + this->hitBox->getHeight()) {
    newYPosition.y = terrainHeight.maxHeight + this->hitBox->getHeight();
  }

  mesh->getPosition()->set(newYPosition);
}

u8 Player::updatePosition(std::vector<Chunck*> loadedChunks,
                          const float& deltaTime, const Vec4& nextPlayerPos,
                          u8 isColliding) {
  Vec4 currentPlayerPos = *this->mesh->getPosition();
  Vec4 playerMin = Vec4();
  Vec4 playerMax = Vec4();
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
      // is vertically out of range?
      if (playerBB.getBottomFace().axisPosition >=
              loadedChunks[chunkIndex]->blocks[i]->maxCorner.y ||
          playerBB.getTopFace().axisPosition <
              loadedChunks[chunkIndex]->blocks[i]->minCorner.y) {
        continue;
      };

      Vec4 tempInflatedMin = Vec4();
      Vec4 tempInflatedMax = Vec4();
      Utils::GetMinkowskiSum(playerMin, playerMax,
                             loadedChunks[chunkIndex]->blocks[i]->minCorner,
                             loadedChunks[chunkIndex]->blocks[i]->maxCorner,
                             &tempInflatedMin, &tempInflatedMax);

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
      if (isColliding) return 0;

      // Try to move in separated axis;
      Vec4 moveOnXOnly =
          Vec4(nextPlayerPos.x, currentPlayerPos.y, currentPlayerPos.z);
      u8 couldMoveOnX =
          this->updatePosition(loadedChunks, deltaTime, moveOnXOnly, 1);
      if (couldMoveOnX) return 1;

      Vec4 moveOnZOnly =
          Vec4(currentPlayerPos.x, nextPlayerPos.y, currentPlayerPos.z);
      u8 couldMoveOnZ =
          this->updatePosition(loadedChunks, deltaTime, moveOnZOnly, 1);
      if (couldMoveOnZ) return 1;

      return 0;
    }
  }

  // Apply new position;
  mesh->getPosition()->x = nextPlayerPos.x;
  mesh->getPosition()->z = nextPlayerPos.z;
  return 1;
}

TerrainHeightModel Player::getTerrainHeightAtPosition(
    std::vector<Chunck*> loadedChunks) {
  TerrainHeightModel model;
  BBox playerBB = this->getHitBox();
  Vec4 minPlayer, maxPlayer;
  playerBB.getMinMax(&minPlayer, &maxPlayer);

  this->currentBlock = nullptr;

  for (size_t chunkIndex = 0; chunkIndex < loadedChunks.size(); chunkIndex++) {
    for (size_t i = 0; i < loadedChunks[chunkIndex]->blocks.size(); i++) {
      // Broad phase
      if (loadedChunks[chunkIndex]->blocks[i]->getPosition()->distanceTo(
              *this->getPosition()) > DUBLE_BLOCK_SIZE)
        continue;

      // Is on block?
      if (!(minPlayer.x < loadedChunks[chunkIndex]->blocks[i]->maxCorner.x &&
            maxPlayer.x > loadedChunks[chunkIndex]->blocks[i]->minCorner.x &&
            minPlayer.z < loadedChunks[chunkIndex]->blocks[i]->maxCorner.z &&
            maxPlayer.z > loadedChunks[chunkIndex]->blocks[i]->minCorner.z))
        continue;

      const float underBlockHeight =
          loadedChunks[chunkIndex]->blocks[i]->maxCorner.y;
      if (minPlayer.y >= underBlockHeight &&
          underBlockHeight > model.minHeight) {
        model.minHeight = underBlockHeight;
        this->currentBlock = loadedChunks[chunkIndex]->blocks[i];
      }

      const float overBlockHeight =
          loadedChunks[chunkIndex]->blocks[i]->minCorner.y;
      if (maxPlayer.y <= overBlockHeight && overBlockHeight < model.maxHeight) {
        model.maxHeight = overBlockHeight;
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
  if (selectedInventoryIndex < 0) selectedInventoryIndex = INVENTORY_SIZE - 1;
  selectedSlotHasChanged = 1;
}

void Player::moveSelectorToTheRight() {
  selectedInventoryIndex++;
  if (selectedInventoryIndex > INVENTORY_SIZE - 1) selectedInventoryIndex = 0;
  selectedSlotHasChanged = 1;
}

void Player::loadMesh() {
  ObjLoaderOptions options;
  options.scale = 15.0F;
  options.flipUVs = true;
  options.animation.count = 10;

  auto data = ObjLoader::load(
      FileUtils::fromCwd("meshes/player/model/player.obj"), options);
  data.get()->loadNormals = false;

  this->mesh = std::make_unique<DynamicMesh>(data.get());

  this->mesh->rotation.identity();
  this->mesh->rotation.rotateY(-3.14F);

  this->mesh->scale.identity();

  this->t_renderer->getTextureRepository().addByMesh(
      this->mesh.get(), FileUtils::fromCwd("meshes/player/"), "png");

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
      ObjLoader::load(FileUtils::fromCwd("meshes/player/arm/arm.obj"), options);
  data.get()->loadNormals = false;

  this->armMesh = std::make_unique<DynamicMesh>(data.get());

  this->armMesh->scale.identity();
  this->armMesh->scale.scaleX(0.7F);
  this->armMesh->scale.scaleZ(1.1F);

  this->armMesh->translation.identity();
  this->armMesh->translation.translateZ(-13.5F);
  this->armMesh->translation.translateY(-8.0F);
  this->armMesh->translation.translateX(5.0F);

  this->armMesh->rotation.identity();
  this->armMesh->rotation.rotateY(-3.24);

  this->t_renderer->getTextureRepository().addByMesh(
      this->armMesh.get(), FileUtils::fromCwd("meshes/player/"), "png");

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
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        this->t_blockManager->getStepSoundByBlockType(blockType);

    if (blockSfxModel != nullptr) {
      this->t_soundManager->setSfxVolume(WALK_SFX_VOL, WALK_SFX_CH);
      this->t_soundManager->playSfx(blockSfxModel->category,
                                    blockSfxModel->sound, WALK_SFX_CH);
    }
  }
}

void Player::toggleFlying() {
  this->isFlying = !this->isFlying;
  if (this->isFlying) {
    this->t_renderer->core.renderer3D.setFov(70.0F);
    this->isOnGround = false;
  } else {
    this->t_renderer->core.renderer3D.setFov(60.0F);
  }
}

void Player::selectNextItem() {
  int currentItemId = (int)inventory[selectedInventoryIndex];
  ItemId nextItem;

  if ((currentItemId + 1) >= (int)ItemId::wooden_axe) {
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
    previousItem = ItemId::chiseled_stone_bricks;
  } else {
    previousItem = static_cast<ItemId>(currentItemId - 1);
  }

  inventory[selectedInventoryIndex] = previousItem;
  this->inventoryHasChanged = true;
}

void Player::jump() {
  this->velocity += this->lift * this->speed;
  this->isOnGround = false;
}

void Player::setRenderPip(PlayerRenderPip* pipToSet) {
  delete this->renderPip;
  this->renderPip = pipToSet;
}