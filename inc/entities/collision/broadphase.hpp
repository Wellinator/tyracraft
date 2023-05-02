// Based on http://allenchou.net/2013/12/game-physics-broadphase/

#pragma once

#include "constants.hpp"
#include <tyra>
#include <math.h>

using Tyra::BBox;
using Tyra::Vec4;

typedef std::pair<Collider*, Collider*> ColliderPair;
typedef std::list<ColliderPair> ColliderPairList;

class Broadphase {
 public:
  // adds a new BBox to the broadphase
  virtual void Add(BBox* aabb) = 0;

  // updates broadphase to react to changes to BBox
  virtual void Update(void) = 0;

  // returns a list of possibly colliding colliders
  virtual const ColliderPairList& ComputePairs(void) = 0;

  // returns a collider that collides with a point
  // returns null if no such collider exists
  virtual Collider* Pick(const Vec3& point) const = 0;

  // returns a list of colliders whose BBoxs collide
  // with a query BBox
  typedef std::vector<Collider*> ColliderList;
  virtual void Query(const BBox& aabb, ColliderList& output) const = 0;

  // result contains the first collider the ray hits
  // result contains null if no collider is hit
  virtual RayCastResult RayCast(const Ray3& ray) const = 0;
};