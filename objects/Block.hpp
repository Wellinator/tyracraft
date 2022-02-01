#ifndef _BLOCK_
#define _BLOCK_

#include "../camera.hpp"
#include <tamtypes.h>
#include <modules/pad.hpp>
#include <modules/timer.hpp>
#include <modules/texture_repository.hpp>
#include "../include/contants.hpp"

/** Block 3D object class  */
class Block
{
public:
    Mesh mesh;
    bool isHidden;
    int block_type;

    Block();
    ~Block();
    void init(Mesh &mother, float X, float Y, float Z);
    bool shouldBeDrawn();
private:

};

#endif
