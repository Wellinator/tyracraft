/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#ifndef _PLAYER_
#define _PLAYER_

#include "camera.hpp"
#include "contants.hpp"
#include "entities/Block.hpp"
#include "entities/item.hpp"
#include <pad/pad.hpp>
#include <math/vec4.hpp>
#include <time/timer.hpp>
#include <physics/ray.hpp>
#include <tamtypes.h>
#include <audio/audio.hpp>
#include "managers/items_repository.hpp"
#include "managers/collision_manager.hpp"

/** Player 3D object class  */
class Player
{

public:
    Mesh mesh;
    Player(Audio *t_audio, TextureRepository *t_texRepo);
    ~Player();

    void update(float deltaTime, const Pad &t_pad, const Camera &t_camera, Block *t_blocks[], unsigned int blocks_ammount);
    inline const Vec4 &getPosition() const { return mesh.position; }
    u8 isFighting, isWalking, isOnGround;
    Vec4 spawnArea;

    //Phisycs variables
    Ray ray;
    Vec4 *nextPlayerPos;
    Block *currentBlock, *willCollideBlock;
    Vec4 willCollideBlockMin, willCollideBlockMax, hitPosition;
    float distanceToHit = -1.0f;

    // Inventory
    u8 inventoryHasChanged = 1;
    u8 selectedSlotHasChanged = 0;
    ITEM_TYPES getSelectedInventoryItemType();
    u8 getSelectedInventorySlot();
    ITEM_TYPES *getInventoryData() { return inventory; };

private:
    TextureRepository *texRepo;
    Vec4 *getNextPosition(float deltaTime, const Pad &t_pad, const Camera &t_camera);
    u8 isWalkingAnimationSet, isJumpingAnimationSet, isFightingAnimationSet;
    u8 isOnBlock, isUnderBlock;
    Audio *audio;
    Timer walkTimer, fightTimer;
    audsrv_adpcm_t *walkAdpcm, *jumpAdpcm, *boomAdpcm;

    //Forces values
    float speed = 100;
    Vec4 velocity, lift;
    
    void getMinMax(const Mesh &t_mesh, Vec4 &t_min, Vec4 &t_max);
    void updatePosition(float deltaTime, const Pad &t_pad, const Camera &t_camera);
    void updateGravity(float deltaTime);
    void checkIfWillCollideBlock(Block *t_blocks[], int blocks_ammount);
    void checkIfIsOnBlock(Block *t_blocks[], int blocks_ammount);
    void handleInputCommands(const Pad &t_pad);

    // Inventory

    ITEM_TYPES inventory[INVENTORY_SIZE] = {
        ITEM_TYPES::dirt,
        ITEM_TYPES::stone,
        ITEM_TYPES::sand,
        ITEM_TYPES::bricks,
        ITEM_TYPES::glass,
        ITEM_TYPES::oak_planks,
        ITEM_TYPES::spruce_planks,
        ITEM_TYPES::stone_brick,
        ITEM_TYPES::chiseled_stone_bricks}; // Starts from 0
    short int selectedInventoryIndex = 0;
    void moveSelectorToTheLeft();
    void moveSelectorToTheRight();
};

#endif
