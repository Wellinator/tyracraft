// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#ifndef _COLLISION_MANAGER_
#define _COLLISION_MANAGER_

#include <utils/debug.hpp>
#include <models/mesh.hpp>
#include <models/sprite.hpp>
#include <models/bounding_box.hpp>
#include <models/math/vector3.hpp>
#include <math.h>
#include "./include/contants.hpp"

enum eDirection
{
    RIGHT,
    LEFT,
    UP,
    DOWN
};

struct Box
{
    Box(){};
    Box(const Vector3 &t_min, const Vector3 &t_max)
    {
        min = t_min;
        max = t_max;
    };

    Vector3 min;
    Vector3 max;
};

class CollisionManager
{

public:
    CollisionManager();
    ~CollisionManager();

    static u8 isColliding(Mesh *meshA, Mesh *meshB);
    static u8 willCollide(Mesh *meshA, Mesh *meshB, const Vector3 *nextPos);
    static u8 willCollideAt(Mesh *meshA, Mesh *meshB, const Vector3 *nextPos);
    static void getMinMaxBoundingBoxAtPos(Vector3 *min, Vector3 *max, Mesh *t_mesh, const Vector3 *nextPos);
    // static u8 isColliding(const BoundingBox &object, const BoundingBox &other);
    // static BoundingBox getSweptBroadphaseRect(const BoundingBox &object);
    // static float sweptAABB(const BoundingBox &object, const BoundingBox &other, eDirection &result);
};

#endif
