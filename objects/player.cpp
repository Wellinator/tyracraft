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

#include <loaders/md2_loader.hpp>
#include <loaders/bmp_loader.hpp>
#include <modules/gif_sender.hpp>
#include <utils/debug.hpp>
#include <utils/math.hpp>

// ----
// Constructors/Destructors
// ----

Player::Player(Audio *t_audio, TextureRepository *t_texRepo)
{
    consoleLog("Creating player object");
    texRepo = t_texRepo;
    audio = t_audio;
    gravity = 0.1F;
    lift = -1.0F;
    jumpCounter = 0;
    speed = 1.0F;
    isWalking = false;
    isFighting = false;
    isWalkingAnimationSet = false;
    isJumpingAnimationSet = false;
    isFightingAnimationSet = false;
    mesh.loadMD2("meshes/player/", "warrior", 0.4F, true);

    //Set player in the middle of the world
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

    consoleLog("Player object created!");
}

Player::~Player()
{
}

// ----
// Methods
// ----

void Player::update(const Pad &t_pad, const Camera &t_camera)
{
    Vector3 *nextPos = getNextPosition(t_pad, t_camera);
    updatePosition(t_pad, t_camera, *nextPos);
    //updateGravity();
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

void Player::updatePosition(const Pad &t_pad, const Camera &t_camera, const Vector3 &t_nextPos)
{

    if (t_pad.rJoyH >= 200)
        mesh.rotation.z += 0.08;
    else if (t_pad.rJoyH <= 100)
        mesh.rotation.z -= 0.08;

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
        if (!isWalkingAnimationSet)
        {
            isWalkingAnimationSet = true;
            this->mesh.playAnimation(1, 8);
        }
        if (walkTimer.getTimeDelta() > 8000)
        {
            walkTimer.prime();
            audio->playADPCM(walkAdpcm, 0);
        }
    }
    else
    {
        isWalkingAnimationSet = false;
        mesh.playAnimation(0, 0);
    }

    if (t_pad.isCrossClicked == 1)
    {
        velocity = lift;
        audio->playADPCM(jumpAdpcm);
        jumpCounter++;
    }

    mesh.position.x = t_nextPos.x;
    mesh.position.z = t_nextPos.z;
}

/** Update player position by gravity and update index of current floor */
void Player::updateGravity()
{
    this->velocity += this->gravity;
    this->mesh.position.y -= this->velocity;

    if (this->mesh.position.y >= 60.0F || mesh.position.y < -60.0F)
    {
        this->mesh.position.y = 60.0F;
        this->velocity = 0;
    }
}
