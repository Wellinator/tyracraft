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

#include "../camera.hpp"
#include "../include/contants.hpp"
#include "../objects/Block.hpp"
#include "../objects/item.hpp"
#include <modules/pad.hpp>
#include <models/math/vector3.hpp>
#include <modules/timer.hpp>
#include <tamtypes.h>
#include <modules/audio.hpp>
#include <modules/texture_repository.hpp>
#include "../managers/items_repository.hpp"

struct BlocksCheck
{
    const Block *currentBlock, *willCollideBlock;
    Vector3 currBlockMin, willCollideBlockMin, currBlockMax, willCollideBlockMax;
};

/** Player 3D object class  */
class Player
{

public:
    float velocity, lift;
    Mesh mesh;
    Player(Audio *t_audio, TextureRepository *t_texRepo);
    ~Player();

    void update(const Pad &t_pad, const Camera &t_camera, Block *t_blocks[], unsigned int blocks_ammount);
    const inline u32 &getJumpCount() const { return jumpCounter; }
    inline const Vector3 &getPosition() const { return mesh.position; }
    u8 isFighting, isWalking, isOnGround;
    Vector3 spawnArea;

    // Inventory
    u8 inventoryHasChanged = 1;
    u8 selectedSlotHasChanged = 1;
    ITEM_TYPES getSelectedInventoryItemType();
    u8 getSelectedInventorySlot();
    ITEM_TYPES *getInventoryData() { return inventory; };

private:
    TextureRepository *texRepo;
    u32 jumpCounter;
    Vector3 *getNextPosition(const Pad &t_pad, const Camera &t_camera);
    u8 isWalkingAnimationSet, isJumpingAnimationSet, isFightingAnimationSet;
    u8 isOnBlock, isUnderBlock;
    Audio *audio;
    Timer walkTimer, fightTimer;
    audsrv_adpcm_t *walkAdpcm, *jumpAdpcm, *boomAdpcm;
    float speed;
    void getMinMax(const Mesh &t_mesh, Vector3 &t_min, Vector3 &t_max);
    void updatePosition(const Pad &t_pad, const Camera &t_camera, const Vector3 &nextPos, BlocksCheck *t_blocksCheck);
    void updateGravity(BlocksCheck *t_blocksCheck);
    BlocksCheck *checkBlocks(Block *t_blocks[], int blocks_ammount, const Vector3 &t_nextPos);
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
        ITEM_TYPES::chiseled_stone_bricks
        }; // Starts from 0
    short int selectedInventoryIndex = 0;
    void moveSelectorToTheLeft();
    void moveSelectorToTheRight();
};

#endif
