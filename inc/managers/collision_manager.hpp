// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include <debug/debug.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/2d/sprite/sprite.hpp>
#include <renderer/3d/bbox/bbox.hpp>
#include <math/vec4.hpp>
#include <math.h>
#include "contants.hpp"

using Tyra::Mesh;
using Tyra::Vec4;
using Tyra::BBox;

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
    Box(const Vec4 &t_min, const Vec4 &t_max)
    {
        min = t_min;
        max = t_max;
    };

    Vec4 min;
    Vec4 max;
};

class CollisionManager
{

public:
    CollisionManager();
    ~CollisionManager();

    static u8 isColliding(Mesh *meshA, Mesh *meshB);
    static u8 willCollide(Mesh *meshA, Mesh *meshB, const Vec4 *nextPos);
    // static void getMinMaxBoundingBoxAtPos(Vec4 *min, Vec4 *max, Mesh *t_mesh, const Vec4 *nextPos);
    static float getManhattanDistance(Vec4 &v1, Vec4 &v2);
    // static u8 willCollideAt(Mesh *meshA, Mesh *meshB, const Vec4 *nextPos);
    // static u8 isColliding(const BBox &object, const BBox &other);
    // static BBox getSweptBroadphaseRect(const BBox &object);
    // static float sweptAABB(const BBox &object, const BBox &other, eDirection &result);
};
