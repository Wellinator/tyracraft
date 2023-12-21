#include "managers/collision_manager.hpp"

/**
 * @brief Definition of global AABBTree
 *
 */
bvh_t g_AABBTree = bvh_t();

void CollisionManager_unloadTree() { g_AABBTree.clear(); };