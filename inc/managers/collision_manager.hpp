#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>
#include <3libs/bvh/bvh.h>

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using Tyra::Vec4;

/**
 * @brief Declaration of global AABBTree
 *
 */
extern AABBTree* g_AABBTree;

void CollisionManager_initTree();
void CollisionManager_unloadTree();