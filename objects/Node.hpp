#ifndef _NODE_
#define _NODE_

#include <tamtypes.h>
#include <models/mesh.hpp>
#include "../include/contants.hpp"

/** Node 3D object class  */
class Node
{
public:
    Mesh mesh;

    Node();
    ~Node();
private:
};

#endif