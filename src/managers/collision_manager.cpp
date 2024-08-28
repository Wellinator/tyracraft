#include "managers/collision_manager.hpp"

/**
 * @brief Definition of global AABBTree
 *
 */
AABBTree* g_AABBTree = nullptr;

void CollisionManager_initTree() {
  g_AABBTree = new AABBTree();
  g_AABBTree->growth = 8.0f;
};

void CollisionManager_unloadTree() {
  g_AABBTree->clear();
  delete g_AABBTree;
  g_AABBTree = nullptr;
};