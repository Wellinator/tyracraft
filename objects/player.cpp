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
    lift = -4.2F;
    jumpCounter = 0;
    speed = 4.0F;
    isWalking = false;
    isFighting = false;
    isWalkingAnimationSet = false;
    isJumpingAnimationSet = false;
    isFightingAnimationSet = false;
    mesh.loadMD2("meshes/player/", "warrior", 0.35F, true);

    // Set player in the middle of the world
    mesh.position.set(0, BLOCK_SIZE, 0);
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

void Player::update(const Pad &t_pad, const Camera &t_camera, Block *t_blocks[], unsigned int blocks_ammount)
{
    Vector3 *nextPos = getNextPosition(t_pad, t_camera);
    BlocksCheck *blocksCheck = this->checkBlocks(t_blocks, blocks_ammount, *nextPos);
    this->updatePosition(t_pad, t_camera, *nextPos, blocksCheck);
    this->updateGravity(blocksCheck);
    delete blocksCheck;
    delete nextPos;
}

Vector3 *Player::getNextPosition(const Pad &t_pad, const Camera &t_camera)
{
    Vector3 *result = new Vector3(mesh.position);
    Vector3 normalizedCamera = Vector3(t_camera.unitCirclePosition);
    normalizedCamera.normalize();
    normalizedCamera *= speed;

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

void Player::updatePosition(const Pad &t_pad, const Camera &t_camera, const Vector3 &t_nextPos, BlocksCheck *t_blocksCheck)
{

    if (t_pad.rJoyH >= 200)
        mesh.rotation.z -= 0.08;
    else if (t_pad.rJoyH <= 100)
        mesh.rotation.z += 0.08;

    isWalking = mesh.position.x != t_nextPos.x || mesh.position.z != t_nextPos.z;
    isFighting = isFighting || t_pad.isCircleClicked;

    if (isFighting)
    {
        if (!isFightingAnimationSet)
        {
            isFightingAnimationSet = true;
            mesh.playAnimation(8, 19, 0);
            fightTimer.prime();
        }
        if (fightTimer.getTimeDelta() > 25000)
        { // end of fighting
            isFightingAnimationSet = false;
            isFighting = false;
        }
    }
    else if (isWalking)
    {
        // if (!isWalkingAnimationSet)
        // {
        //     isWalkingAnimationSet = true;
        //     this->mesh.playAnimation(1, 8);
        // }
        if (walkTimer.getTimeDelta() > 8000)
        {
            walkTimer.prime();
            audio->playADPCM(walkAdpcm, 0);
        }
    }
    // else
    // {
    //     isWalkingAnimationSet = false;
    //     mesh.playAnimation(0, 0);
    // }

    if (t_pad.isCrossClicked == 1 && this->isOnGround)
    {
        velocity = lift;
        // audio->playADPCM(jumpAdpcm);
        jumpCounter++;
        this->isOnGround = 0;
    }

    if (t_blocksCheck->willCollideBlock == NULL)
    {
        mesh.position.x = t_nextPos.x;
        mesh.position.z = t_nextPos.z;
    }
}

/** Update player position by gravity and update index of current block */
void Player::updateGravity(BlocksCheck *t_blocksCheck)
{
    this->velocity += GRAVITY;
    this->mesh.position.y -= this->velocity;
    this->isOnBlock = t_blocksCheck->currentBlock != NULL && mesh.position.y < t_blocksCheck->currBlockMax.y;

    if (this->mesh.position.y >= OVERWORLD_MAX_HEIGH * DUBLE_BLOCK_SIZE || mesh.position.y < OVERWORLD_MIN_HEIGH * DUBLE_BLOCK_SIZE)
    {
        this->mesh.position.y = t_blocksCheck->currBlockMax.y;
        this->velocity = 0;
    }
    else if (this->isOnBlock)
    {
        this->mesh.position.y = t_blocksCheck->currBlockMax.y;
        this->velocity = 0;
        this->isOnGround = 1;
    }
}

BlocksCheck *Player::checkBlocks(Block *t_blocks[], int blocks_ammount, const Vector3 &t_nextPos)
{
    BlocksCheck *result = new BlocksCheck();
    result->currentBlock = NULL;
    result->willCollideBlock = NULL;
    Vector3 min = Vector3();
    Vector3 max = Vector3();

    for (int i = 0; i < blocks_ammount; i++)
    {
        if (t_blocks[i]->type != AIR_BLOCK &&
            this->getPosition().distanceTo(t_blocks[i]->position) <= MAX_RANGE_PICKER)
        {
            t_blocks[i]->mesh.getMinMaxBoundingBox(&min, &max);
            if (result->currentBlock == NULL && this->mesh.position.isOnBox(min, max))
            {
                result->currentBlock = t_blocks[i];
                result->currBlockMin.set(min);
                result->currBlockMax.set(max);
            }
            if (result->willCollideBlock == NULL && 
                mesh.position.y < max.y
                && t_nextPos.collidesBox(min, max))
            {
                result->willCollideBlock = t_blocks[i];
                result->willCollideBlockMin.set(min);
                result->willCollideBlockMax.set(max);
            }
            if (result->currentBlock != NULL && result->willCollideBlock != NULL)
                break;
        }
    }
    return result;
}