/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#include "player.hpp"
#include "../utils.hpp"
#include <loaders/mdl_loader.hpp>
#include <loaders/bmp_loader.hpp>
#include <modules/gif_sender.hpp>
#include <utils/debug.hpp>
#include <utils/math.hpp>

// ----
// Constructors/Destructors
// ----

Player::Player(Audio *t_audio, TextureRepository *t_texRepo)
{
    texRepo = t_texRepo;
    audio = t_audio;

    // Phisycs values
    lift = Vector3(0.0f, -250.0F, 0.0f);
    velocity = Vector3(0.0f, 0.0f, 0.0f);

    isWalking = false;
    isFighting = false;
    isWalkingAnimationSet = false;
    isJumpingAnimationSet = false;
    isFightingAnimationSet = false;

    mesh.loadMD2("meshes/player/", "warrior", 0.35F, true);
    mesh.position.set(0.00F, 0.00F, 0.00F);
    mesh.rotation.x = -1.566F;
    mesh.rotation.z = 1.566F;
    mesh.shouldBeBackfaceCulled = false;
    mesh.shouldBeFrustumCulled = false;
    mesh.setAnimSpeed(0.17F);
    mesh.playAnimation(0, 0);

    texRepo->addByMesh("meshes/player/", mesh, BMP);

    walkAdpcm = audio->loadADPCM("sounds/walk.adpcm");
    jumpAdpcm = audio->loadADPCM("sounds/jump.adpcm");
    boomAdpcm = audio->loadADPCM("sounds/boom.adpcm");
    audio->setADPCMVolume(70, 0);
}

Player::~Player()
{
}

// ----
// Methods
// ----

void Player::update(float deltaTime, const Pad &t_pad, const Camera &t_camera, Block *t_blocks[], unsigned int blocks_ammount)
{
    this->handleInputCommands(t_pad);

    this->nextPlayerPos = getNextPosition(deltaTime, t_pad, t_camera);
    this->checkIfWillCollideBlock(t_blocks, blocks_ammount);
    this->checkIfIsOnBlock(t_blocks, blocks_ammount);
    this->updateGravity(deltaTime);
    this->updatePosition(deltaTime, t_pad, t_camera);

    delete nextPlayerPos;
    nextPlayerPos = NULL;
}

void Player::handleInputCommands(const Pad &t_pad)
{
    if (t_pad.isL1Clicked)
        this->moveSelectorToTheLeft();
    if (t_pad.isR1Clicked)
        this->moveSelectorToTheRight();
    if (t_pad.isCrossClicked && this->isOnGround)
    {
        this->velocity = this->lift;
        this->isOnGround = 0;
        // audio->playADPCM(jumpAdpcm);
    }

    if (t_pad.rJoyH >= 200)
        this->mesh.rotation.z -= 0.08;
    else if (t_pad.rJoyH <= 100)
        this->mesh.rotation.z += 0.08;
}

Vector3 *Player::getNextPosition(float deltaTime, const Pad &t_pad, const Camera &t_camera)
{
    Vector3 *result = new Vector3(mesh.position);
    Vector3 normalizedCamera;
    normalizedCamera.set(t_camera.unitCirclePosition);
    normalizedCamera.normalize();
    normalizedCamera *= (this->speed * deltaTime);

    if (t_pad.lJoyV <= 100)
    {
        result->x += -normalizedCamera.x;
        result->z += -normalizedCamera.z;
    }
    else if (t_pad.lJoyV >= 200)
    {
        result->x += normalizedCamera.x;
        result->z += normalizedCamera.z;
    }
    if (t_pad.lJoyH <= 100)
    {
        result->x += -normalizedCamera.z;
        result->z += normalizedCamera.x;
    }
    else if (t_pad.lJoyH >= 200)
    {
        result->x += normalizedCamera.z;
        result->z += -normalizedCamera.x;
    }

    return result;
}

void Player::updatePosition(float deltaTime, const Pad &t_pad, const Camera &t_camera)
{
    // Check if is at the world's edge
    if (!nextPlayerPos->collidesBox(MIN_WORLD_POS, MAX_WORLD_POS))
        return;

    // Will collide
    if (distanceToHit > -1.0f)
    {
        float timeToHit = distanceToHit / this->speed;
        if (timeToHit <= deltaTime || distanceToHit < this->mesh.position.distanceTo(*nextPlayerPos))
            return;
    }

    // Apply new position;
    mesh.position.x = nextPlayerPos->x;
    mesh.position.z = nextPlayerPos->z;
}

/** Update player position by gravity and update index of current block */
void Player::updateGravity(float deltaTime)
{
    this->velocity += GRAVITY; // Negative gravity to decrease Y axis
    Vector3 newYPosition = mesh.position - (this->velocity * deltaTime);

    if (newYPosition.y >= OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE || newYPosition.y < OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE)
    {
        // Maybe has died, teleport to spaw area
        printf("\nReseting player position to:\n");
        this->spawnArea.print();
        this->mesh.position.set(this->spawnArea);
        this->velocity = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    if (this->currentBlock != NULL && newYPosition.y < this->currentBlock->maxCorner.y)
    {
        newYPosition.y = this->currentBlock->maxCorner.y;
        this->velocity = Vector3(0.0f, 0.0f, 0.0f);
        this->isOnGround = 1;
    }

    // Finally updates gravity after checks
    mesh.position.set(newYPosition);
}

void Player::checkIfWillCollideBlock(Block *t_blocks[], int blocks_ammount)
{
    this->distanceToHit = -1.0f;

    // Get the direction
    Vector3 rayDir = *nextPlayerPos - this->mesh.position;
    rayDir.normalize();
    ray.set(this->mesh.position, rayDir);

    float finalHitDistance = -1.0f;
    float tempHitDistance = -1.0f;

    for (int i = 0; i < blocks_ammount; i++)
    {
        if (CollisionManager::getManhattanDistance(this->mesh.position, t_blocks[i]->position) <= (MAX_RANGE_PICKER / 2))
        {
            // Project ray
            ray.intersectBox(&t_blocks[i]->minCorner, &t_blocks[i]->maxCorner, tempHitDistance);

            if (tempHitDistance > 0)
            {
                if (finalHitDistance == -1.0f)
                    finalHitDistance = tempHitDistance;
                else if (tempHitDistance < finalHitDistance)
                    finalHitDistance = tempHitDistance;
            }
        }
    }

    if (finalHitDistance > -1.0f)
    {
        this->hitPosition.set(this->ray.at(finalHitDistance));
        this->distanceToHit = finalHitDistance;
    }
}

void Player::checkIfIsOnBlock(Block *t_blocks[], int blocks_ammount)
{
    this->currentBlock = NULL;

    for (int i = 0; i < blocks_ammount; i++)
    {
        float distanceToBlock = CollisionManager::getManhattanDistance(this->mesh.position, t_blocks[i]->position);
        if (distanceToBlock <= (MAX_RANGE_PICKER / 4))
        {
            if (this->mesh.position.isOnBox(t_blocks[i]->minCorner, t_blocks[i]->maxCorner))
            {
                if (this->currentBlock == NULL)
                {
                    this->currentBlock = t_blocks[i];
                    continue;
                }

                if (distanceToBlock < CollisionManager::getManhattanDistance(this->mesh.position, this->currentBlock->position))
                    this->currentBlock = t_blocks[i];
            }
        }
    }
}

/**
 * Inventory controllers
 *
 */

ITEM_TYPES Player::getSelectedInventoryItemType()
{
    return this->inventory[this->selectedInventoryIndex];
}

/**
 * @brief Return selected slot - int between 1 and 9
 *
 */
u8 Player::getSelectedInventorySlot()
{
    return this->selectedInventoryIndex + 1;
}

void Player::moveSelectorToTheLeft()
{
    selectedInventoryIndex--;
    if (selectedInventoryIndex < 0)
        selectedInventoryIndex = INVENTORY_SIZE - 1;
    selectedSlotHasChanged = 1;
}

void Player::moveSelectorToTheRight()
{
    selectedInventoryIndex++;
    if (selectedInventoryIndex > INVENTORY_SIZE - 1)
        selectedInventoryIndex = 0;
    selectedSlotHasChanged = 1;
}
