#include "managers/collision_manager.hpp"

/**
 * @brief Definition of global AABBTree
 *
 */
AABBTree* g_AABBTree = nullptr;

void CollisionManager_initTree() { g_AABBTree = new AABBTree(); };

void CollisionManager_unloadTree() {
  delete g_AABBTree;
  g_AABBTree = nullptr;
};