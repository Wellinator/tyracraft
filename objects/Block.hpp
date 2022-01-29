#ifndef _BLOCK_
#define _BLOCK_

#include "../camera.hpp"
#include <tamtypes.h>
#include <modules/pad.hpp>
#include <modules/timer.hpp>
#include <modules/texture_repository.hpp>

/** Block 3D object class  */
class Block
{
public:
    Mesh mesh;
    Block(TextureRepository *t_texRepo, float X, float Y, float Z);
    ~Block();

private:
    TextureRepository *texRepo;
};

#endif
