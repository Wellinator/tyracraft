#include "entities/player.hpp"

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

  isWalking = false;
  isWalkingAnimationSet = false;
  isBreakingAnimationSet = false;

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
                    std::vector<Block*> loadedBlocks) {
  this->shouldRenderPlayerModel = t_camera.getCamType() == CamType::ThirdPerson;

  this->handleInputCommands(t_pad);

  const Vec4 nextPlayerPos = getNextPosition(deltaTime, t_pad, t_camera);

  if (nextPlayerPos.collidesBox(MIN_WORLD_POS, MAX_WORLD_POS)) {
    const u8 hasMoved = this->updatePosition(
        &loadedBlocks[0], loadedBlocks.size(), deltaTime, nextPlayerPos);

    const u8 canPlayWalkSound = !t_pad.getLeftJoyPad().isCentered && hasMoved &&
                                this->isOnGround &&
                                this->currentBlock != nullptr;
    if (canPlayWalkSound) this->playWalkSfx(this->currentBlock->type);
  }

  float terrainHeight = this->getTerrainHeightOnPlayerPosition(
      &loadedBlocks[0], loadedBlocks.size());
  this->updateGravity(deltaTime, terrainHeight);

  // this->handledItem->mesh->translation.identity();
  // this->handledItem->mesh->translation.operator*=(this->mesh->translation);
}

void Player::render() {
  if (shouldRenderPlayerModel) {
    this->t_renderer->renderer3D.usePipeline(&dynpip);
    dynpip.render(mesh.get(), &modelDynpipOptions);
  }

  if (this->getSelectedInventoryItemType() == ItemId::empty) {
    this->t_renderer->renderer3D.usePipeline(&dynpip);
    dynpip.render(armMesh.get(), &armDynpipOptions);
  }

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

void Player::handleInputCommands(Pad& t_pad) {
  if (t_pad.getClicked().L1) this->moveSelectorToTheLeft();
  if (t_pad.getClicked().R1) this->moveSelectorToTheRight();

  if (t_pad.getPressed().L2) {
    if (!isBreakingAnimationSet) {
      this->mesh->animation.speed = baseAnimationSpeed * 3;
      this->mesh->animation.setSequence(breakBlockSequence);

      this->armMesh->animation.speed = baseAnimationSpeed * 6;
      this->armMesh->animation.setSequence(armHitingSequence);

      isBreakingAnimationSet = true;
    }
  } else {
    isBreakingAnimationSet = false;
  }

  if (t_pad.getClicked().Cross && this->isOnGround) {
    this->velocity += this->lift * this->speed;
    this->isOnGround = 0;
  }

  // FIXME: Player mesh rotation based on camera direction 
  // if (t_pad.getRightJoyPad().h >= 200) {
  //   this->mesh->rotation.rotateY(-0.08F);
  // } else if (t_pad.getRightJoyPad().h <= 100) {
  //   this->mesh->rotation.rotateY(0.08F);
  // }

  if (t_pad.getLeftJoyPad().isMoved) {
    if (!isWalkingAnimationSet) {
      this->mesh->animation.speed = baseAnimationSpeed;
      this->mesh->animation.setSequence(walkSequence);

      this->armMesh->animation.speed = baseAnimationSpeed * 3;
      this->armMesh->animation.setSequence(armWalkingSequence);

      isWalkingAnimationSet = true;
    }
  } else {
    isWalkingAnimationSet = false;
  }

  u8 isAnimating = (isBreakingAnimationSet || isWalkingAnimationSet);
  if (!isAnimating) {
    if (!isStandStillAnimationSet) {
      this->mesh->animation.setSequence(standStillSequence);
      this->armMesh->animation.setSequence(armStandStillSequence);
      isStandStillAnimationSet = true;
    }
  } else {
    isStandStillAnimationSet = false;
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
void Player::updateGravity(const float& deltaTime, const float terrainHeight) {
  this->velocity += GRAVITY;  // Negative gravity to decrease Y axis
  Vec4 newYPosition = *mesh->getPosition() -
                      (this->velocity * std::min(deltaTime, MAX_FRAME_MS));

  if (newYPosition.y >= OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE ||
      newYPosition.y < OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE) {
    // Maybe has died, teleport to spaw area
    printf("\nReseting player position to:\n");
    // FIXME: reset loaded chunks to spawn position;
    this->spawnArea.print();
    this->mesh->getPosition()->set(this->spawnArea);
    this->velocity = Vec4(0.0f, 0.0f, 0.0f);
    return;
  }

  if (newYPosition.y < terrainHeight) {
    newYPosition.y = terrainHeight;
    this->velocity = Vec4(0.0f, 0.0f, 0.0f);
    this->isOnGround = 1;
  }

  // Finally updates gravity after checks
  mesh->getPosition()->set(newYPosition);
}

u8 Player::updatePosition(Block** t_blocks, int blocks_ammount,
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

  for (int i = 0; i < blocks_ammount; i++) {
    // Broad phase
    // is vertically out of range?
    if (playerBB.getBottomFace().axisPosition >= t_blocks[i]->maxCorner.y ||
        playerBB.getTopFace().axisPosition < t_blocks[i]->minCorner.y) {
      continue;
    };

    Vec4 tempInflatedMin = Vec4();
    Vec4 tempInflatedMax = Vec4();
    Utils::GetMinkowskiSum(playerMin, playerMax, t_blocks[i]->minCorner,
                           t_blocks[i]->maxCorner, &tempInflatedMin,
                           &tempInflatedMax);

    if (ray.intersectBox(tempInflatedMin, tempInflatedMax, &tempHitDistance)) {
      // Is horizontally collidable?
      if (tempHitDistance > maxCollidableDistance) continue;

      if (finalHitDistance == -1.0f || tempHitDistance < finalHitDistance) {
        finalHitDistance = tempHitDistance;
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
      u8 couldMoveOnX = this->updatePosition(t_blocks, blocks_ammount,
                                             deltaTime, moveOnXOnly, 1);
      if (couldMoveOnX) return 1;

      Vec4 moveOnZOnly =
          Vec4(currentPlayerPos.x, nextPlayerPos.y, currentPlayerPos.z);
      u8 couldMoveOnZ = this->updatePosition(t_blocks, blocks_ammount,
                                             deltaTime, moveOnZOnly, 1);
      if (couldMoveOnZ) return 1;

      return 0;
    }
  }

  // Apply new position;
  mesh->getPosition()->x = nextPlayerPos.x;
  mesh->getPosition()->z = nextPlayerPos.z;
  return 1;
}

float Player::getTerrainHeightOnPlayerPosition(Block** t_blocks,
                                               int blocks_ammount) {
  float higherY = (OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE);
  BBox playerBB = this->getHitBox();
  Vec4 minPlayer, maxPlayer;
  playerBB.getMinMax(&minPlayer, &maxPlayer);

  this->currentBlock = nullptr;

  for (int i = 0; i < blocks_ammount; i++) {
    const float blockHeight = t_blocks[i]->maxCorner.y;

    if (playerBB.getBottomFace().axisPosition < blockHeight) continue;

    float distanceToBlock =
        mesh.get()->getPosition()->distanceTo(*t_blocks[i]->getPosition());

    if (distanceToBlock <= MAX_RANGE_PICKER) {
      u8 isOnBlock = minPlayer.x < t_blocks[i]->maxCorner.x &&
                     maxPlayer.x > t_blocks[i]->minCorner.x &&
                     minPlayer.z < t_blocks[i]->maxCorner.z &&
                     maxPlayer.z > t_blocks[i]->minCorner.z;

      if (isOnBlock) {
        if (blockHeight > higherY) {
          higherY = blockHeight;
          this->currentBlock = t_blocks[i];
        }
      }
    }
  }

  return higherY;
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
  this->armMesh->translation.translateZ(-15.0F);
  this->armMesh->translation.translateY(-8.0F);
  this->armMesh->translation.translateX(5.0F);

  this->armMesh->rotation.identity();
  this->armMesh->rotation.rotateY(-3.14F);

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

void Player::playWalkSfx(u8 blockType) {
  if (blockType != AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        this->t_blockManager->getStepSoundByBlockType(blockType);

    if (blockSfxModel != nullptr)
      this->t_soundManager->playSfx(blockSfxModel->category,
                                    blockSfxModel->sound, WALK_SFX_CH);
  }
}
