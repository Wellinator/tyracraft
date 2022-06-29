#include "collision_manager.hpp"

CollisionManager::CollisionManager() {}

CollisionManager::~CollisionManager() {}

/** @returns check's intersection against another AABB */
u8 CollisionManager::isColliding(Mesh *meshA, Mesh *meshB)
{
    const BoundingBox *boxA = meshA->getCurrentBoundingBox();
    const BoundingBox *boxB = meshB->getCurrentBoundingBox();

    float left = (boxB->getLeftFace().axisPosition + meshB->position.x) - (boxA->getRightFace().axisPosition + meshA->position.x);
    float right = (boxB->getRightFace().axisPosition + meshB->position.x) - (boxA->getLeftFace().axisPosition + meshA->position.x);
    float top = (boxB->getTopFace().axisPosition + meshB->position.y) - (boxA->getBottomFace().axisPosition + meshA->position.y);
    float bottom = (boxB->getBottomFace().axisPosition + meshB->position.y) - (boxA->getTopFace().axisPosition + meshA->position.y);
    float front = (boxB->getFrontFace().axisPosition + meshB->position.z) - (boxA->getBackFace().axisPosition + meshA->position.z);
    float back = (boxB->getBackFace().axisPosition + meshB->position.z) - (boxA->getFrontFace().axisPosition + meshA->position.z);

    return !(left > 0 || right < 0 || top < 0 || bottom > 0 || back > 0 || front < 0);
}

u8 CollisionManager::willCollide(Mesh *meshA, Mesh *meshB, const Vector3 *nextPos)
{
    const BoundingBox *boxA = meshA->getCurrentBoundingBox();
    const BoundingBox *boxB = meshB->getCurrentBoundingBox();

    if (((boxA->getLeftFace().axisPosition + meshB->position.x) <= (boxB->getRightFace().axisPosition + nextPos->x) && (boxA->getRightFace().axisPosition + meshB->position.x) >= (boxB->getLeftFace().axisPosition + nextPos->x)) &&
        ((boxA->getTopFace().axisPosition + meshB->position.y) <= (boxB->getBottomFace().axisPosition + nextPos->y) && (boxA->getBottomFace().axisPosition + meshB->position.y) >= (boxB->getTopFace().axisPosition + nextPos->y)) &&
        ((boxA->getFrontFace().axisPosition + meshB->position.z) <= (boxB->getBackFace().axisPosition + nextPos->z) && (boxA->getBackFace().axisPosition + meshB->position.z) >= (boxB->getFrontFace().axisPosition + nextPos->z)))
        return 0;
    return 1;
}

/** @returns check's intersection against another AABB */
u8 CollisionManager::willCollideAt(Mesh *meshA, Mesh *meshB, const Vector3 *nextPos)
{
    Vector3 aMin;
    Vector3 aMax;
    Vector3 bMin;
    Vector3 bMax;

    CollisionManager::getMinMaxBoundingBoxAtPos(&aMin, &aMax, meshA, nextPos);
    meshB->getMinMaxBoundingBox(&bMin, &bMax);

    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
           (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
           (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

// /** @returns check's intersection against another AABB */
// u8 CollisionManager::isColliding(Mesh *meshA, Mesh *meshB)
// {
//     const Vector3 aMin;
//     const Vector3 aMax;
//     const Vector3 bMin;
//     const Vector3 bMax;

//     meshA->getMinMaxBoundingBox(&aMin, &aMax);
//     meshB->getMinMaxBoundingBox(&bMin, &bMax);

//     return (amin.x <= bMax.x && aMax.x >= bMin.x) &&
//            (amin.y <= bMax.y && aMax.y >= bMin.y) &&
//            (amin.z <= bMax.z && aMax.z >= bMin.z);
// }

// BoundingBox CollisionManager::getSweptBroadphaseBox(const BoundingBox &box)
// {
//     float x = box.vx > 0 ? box.x : box.x + box.vx;
//     float y = box.vy > 0 ? box.y : box.y + box.vy;
//     float w = box.width + abs(box.vx);
//     float h = box.height + abs(box.vy);

//     return BoundingBox(x, y, w, h);
// }

// float CollisionManager::sweptAABB(const BoundingBox &object, const BoundingBox &other, eDirection &result)
// {
//     float dxEntry, dxExit;
//     float dxEntry, dxExit;

//     if (object.vx > 0.0f)
//     {
//         dxEntry = other.x - (object.x + object.width);
//         dxExit = (other.x + other.width) - object.x;
//     }
//     else
//     {
//         dxEntry = (other.x + other.width) - object.x;
//         dxExit = other.x - (object.x + object.width);
//     }

//     if (object.vy > 0.0f)
//     {
//         dyEntry = other.y - (object.y + object.height);
//         dyExit = (other.y + height) - object.y;
//     }
//     else
//     {
//         dyEntry = (other.y + other.height) - object.y;
//         dyExit = other.y - (object.y + object.height);
//     }

//     if (object.vx == 0.0f)
//     {
//         txEntry = -std::numeric_limits<float>::infinity();
//         txExit = std::numeric_limits<float>::infinity();
//     }
//     else
//     {
//         txEntry = dxEntry / object.vx;
//         txExit = dxExit / object.vx;
//     }

//     if (object.vy == 0.0f)
//     {
//         tyEntry = -std::numeric_limits<float>::infinity();
//         tyExit = std::numeric_limits<float>::infinity();
//     }
//     else
//     {
//         tyEntry = dyEntry / object.vy;
//         tyExit = dyExit / object.vy;
//     }

//     float entryTime = max(txEntry, tyEntry);
//     float exitTime = min(txExit, tyExit);

//     if (entryTime > exitTime || (txEntry < 0.0f && tyEntry < 0.0f) || txEntry > 1.0f || tyEntry > 1.0f)
//     {
//         return 1.0f;
//     }

//     if (txEntry > tyEntry)
//     {
//         if (dxEntry > 0.0f)
//         {
//             result = eDirection::RIGHT;
//         }
//         else
//         {
//             result = eDirection::LEFT;
//         }
//     }
//     else
//     {
//         if (dyEntry > 0.0f)
//         {
//             result = eDirection::UP;
//         }
//         else
//         {
//             result = eDirection::DOWN;
//         }
//     }

//     return entryTime;
// }

void CollisionManager::getMinMaxBoundingBoxAtPos(Vector3 *min, Vector3 *max, Mesh *t_mesh, const Vector3 *nextPos)
{
    Vector3 calc = Vector3();

    u8 isInitialized = 0;
    const Vector3 *boundingBox = t_mesh->getCurrentBoundingBoxVertices();
    for (u8 i = 0; i < 8; i++)
    {
        calc.set(
            boundingBox[i].x + nextPos->x,
            boundingBox[i].y + nextPos->y,
            boundingBox[i].z + nextPos->z);
        if (isInitialized == 0)
        {
            isInitialized = 1;
            min->set(calc);
            max->set(calc);
        }

        if (min->x > calc.x)
            min->x = calc.x;
        if (calc.x > max->x)
            max->x = calc.x;

        if (min->y > calc.y)
            min->y = calc.y;
        if (calc.y > max->y)
            max->y = calc.y;

        if (min->z > calc.z)
            min->z = calc.z;
        if (calc.z > max->z)
            max->z = calc.z;
    }
}

float CollisionManager::getManhattanDistance(Vector3 &v1, Vector3 &v2)
{
    float x_dif, y_dif, z_dif;

    x_dif = v2.x - v1.x;
    y_dif = v2.y - v1.y;
    z_dif = v2.z - v1.z;

    if (x_dif < 0)
        x_dif = -x_dif;
    if (y_dif < 0)
        y_dif = -y_dif;
    if (z_dif < 0)
        z_dif = -z_dif;

    return x_dif + y_dif + z_dif;
}