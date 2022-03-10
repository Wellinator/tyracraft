#ifndef _BLOCK_
#define _BLOCK_

#include <tamtypes.h>
#include <models/math/vector3.hpp>
#include <models/mesh.hpp>
#include "../include/contants.hpp"

/** Block 3D object class  */
class Block
{
public:
    int block_type;
    Mesh mesh;

    Block(int block_type);
    ~Block();
private:
};

#endif
