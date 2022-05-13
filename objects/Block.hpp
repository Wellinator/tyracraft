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
    Mesh mesh;
    
    u8 type = AIR_BLOCK;  //Init as air

    int index;//Index at terrain;
    Vector3 position;//Index at terrain;
    
    //Block state
    u8 isTarget = 0;
    u8 isSolid = 0;
    u8 isEditable = 0;
    u8 visibility = 255;
    u8 isHidden = 1;


    Block(u8 block_type);
    ~Block();
private:
    //Block props
};

#endif
