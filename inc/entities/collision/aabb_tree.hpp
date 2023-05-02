// Based on
// http://allenchou.net/2014/02/game-physics-broadphase-dynamic-aabb-tree/
#pragma once

#include "constants.hpp"
#include "entities/collision/aabb_node.hpp"
#include "entities/collision/broadphase.hpp"
#include <tyra>
#include <math.h>

using Tyra::BBox;
using Tyra::Vec4;

class AABBTree : public Broadphase {
 public:
 public:
  AABBTree(void)
      : m_root(nullptr),
        m_margin(0.2f)  // 20cm
  {}

  virtual void Add(BBox* aabb);
  virtual void Remove(BBox* aabb);
  virtual void Update();
  // virtual ColliderPairList& ComputePairs(void);
  // virtual Collider* Pick(const Vec3& point) const;
  // virtual Query(const BBox& aabb, ColliderList& out) const;
  // virtual RayCastResult RayCast(const Ray3& ray) const;

 private:
  typedef std::vector<Node*> NodeList;

  void UpdateNodeHelper(Node* node, NodeList& invalidNodes);
  void InsertNode(Node* node, Node** parent);
  void RemoveNode(Node* node);
  // void ComputePairsHelper(Node* n0, Node* n1);
  // void ClearChildrenCrossFlagHelper(Node* node);
  // void CrossChildren(Node* node);

  Node* m_root;
  // ColliderPairList m_pairs;
  float m_margin;
  NodeList m_invalidNodes;
};
