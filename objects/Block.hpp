#ifndef _BLOCK_
#define _BLOCK_

#include <tamtypes.h>
#include <models/math/vector3.hpp>
#include "../include/contants.hpp"

/** Block 3D object class  */
class Block
{
public:
    Vector3 position;
    bool isHidden = true;
    int blockType;

    Block();
    ~Block();
    void init(int block_type, float X, float Y, float Z);
    bool shouldBeDrawn();
private:
};

#endif
