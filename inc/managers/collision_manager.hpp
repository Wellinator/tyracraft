#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>
#include <3libs/bvh/bvh.h>

using Tyra::Vec4;
using bvh::aabb_t;
using bvh::bvh_t;
using bvh::node_t;

/**
 * @brief Declaration of global AABBTree
 *
 */
extern bvh_t g_AABBTree;

void CollisionManager_unloadTree();