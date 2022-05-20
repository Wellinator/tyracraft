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
#include <modules/pad.hpp>
#include <models/math/vector3.hpp>
#include <modules/timer.hpp>
#include <tamtypes.h>
#include <modules/audio.hpp>
#include <modules/texture_repository.hpp>

struct BlocksCheck
{
    const Block *currentBlock, *willCollideBlock;
    Vector3 currBlockMin, willCollideBlockMin, currBlockMax, willCollideBlockMax;
};

/** Player 3D object class  */
class Player
{

public:
    float gravity, velocity, lift;
    Mesh mesh;
    Player(Audio *t_audio, TextureRepository *t_texRepo);
    ~Player();

    void update(const Pad &t_pad, const Camera &t_camera, Block *t_blocks[], unsigned int blocks_ammount);
    const inline u32 &getJumpCount() const { return jumpCounter; }
    inline const Vector3 &getPosition() const { return mesh.position; }
    u8 isFighting, isWalking, isOnGround;

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
};

#endif
