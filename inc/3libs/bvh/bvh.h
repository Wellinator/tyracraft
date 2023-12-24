#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <vector>
#include <cstdint>
#include <math.h>
#include <cmath>
#include <tyra>

using Tyra::Ray;
using Tyra::Vec4;

namespace bvh {

using index_t = int32_t;

static const index_t invalid_index = -1;

class AABB {
 public:
  // lower bound
  float minx, miny, minz;
  // upper bound
  float maxx, maxy, maxz;

  bool raycast(float x0, float y0, float x1, float y1) const;
  bool raycast(float x0, float y0, float x1, float y1, const bvh::AABB& aabb) const;
  bool raycast(const Vec4 origin, const Vec4 direction) const;
  bool raycast(const Vec4 origin, const Vec4 direction, const bvh::AABB& aabb) const;
  bool intersectLine(const Vec4 p1, const Vec4 p2) const;
  bool intersectLine(const Vec4 p1, const Vec4 p2, const bvh::AABB& aabb) const;

  inline float area() const {
    const float dx = maxx - minx;
    const float dy = maxy - miny;
    const float dz = maxz - minz;
    return dx * dy * dz;
  }

  // return true if two aabbs overlap
  static bool overlaps(const AABB& a, const AABB& b) {
    if (a.maxx < b.minx) return false;
    if (a.minx > b.maxx) return false;
    if (a.maxy < b.miny) return false;
    if (a.miny > b.maxy) return false;
    if (a.maxz < b.minz) return false;
    if (a.minz > b.maxz) return false;
    return true;
  }

  // find the union of two aabb
  static AABB find_union(const AABB& a, const AABB& b) {
    return AABB{
        std::min<float>(a.minx, b.minx), std::min<float>(a.miny, b.miny),
        std::min<float>(a.minz, b.minz), std::max<float>(a.maxx, b.maxx),
        std::max<float>(a.maxy, b.maxy), std::max<float>(a.maxz, b.maxz)};
  }

  // grow the aabb evenly by an amount
  static AABB grow(const AABB& a, float amount) {
    return AABB{a.minx - amount, a.miny - amount, a.minz - amount,
                a.maxx + amount, a.maxy + amount, a.maxz + amount};
  }

  // evaluate if this aabb contains another
  bool contains(const AABB& a) const {
    return a.minx >= minx && a.miny >= miny && a.minz >= minz &&
           a.maxx <= maxx && a.maxy <= maxy && a.maxz <= maxz;
  }
};

class Bvh_Node {
 public:
  // for a leaf this will be a fat aabb and non terminal nodes will be regular
  AABB aabb;

  // left and right children
  std::array<index_t, 2> child;

  // index of the parent node
  index_t parent;

  // user provided data
  void* user_data;

  // query if this node is a neaf
  bool is_leaf() const { return child[0] == invalid_index; }

  void replace_child(index_t prev, index_t with) {
    if (child[0] == prev) {
      child[0] = with;
    } else {
      assert(child[1] == prev);
      child[1] = with;
    }
  }
};

class AABBTree {
 public:
  AABBTree();
  ~AABBTree();

  // remove all nodes from the tree
  void clear();

  // create a new node in the tree
  index_t insert(AABB aabb, void* user_data);

  // remove a node from the tree
  void remove(index_t index);

  // move an existing node in the tree
  void move(index_t index, const AABB& aabb);

  // return a nodes user data
  void* user_data(index_t index) const {
    assert(index >= 0 && index < index_t(_nodes.size()));
    return _nodes[index].user_data;
  }

  // get a node from the tree
  const Bvh_Node& get(index_t index) const {
    assert(index >= 0 && index < index_t(_nodes.size()));
    return _nodes[index];
  }

  // get the root node of the tree
  const Bvh_Node& root() const {
    assert(_root != invalid_index);
    return _nodes[_root];
  }

  bool empty() const { return _root == invalid_index; }

  // this is the growth size for fat aabbs (they will be expanded by this)
  float growth;

  // find all overlaps with a given bounding-box
  void find_overlaps(const AABB& bb, std::vector<index_t>& overlaps);

  // find all overlaps with a given node
  void find_overlaps(index_t node, std::vector<index_t>& overlaps);

  // find all overlaps with a line segment in 2D
  void raycast(float x0, float y0, float x1, float y1,
               std::vector<index_t>& overlaps);

  // find all overlaps with a line segment in 3D
  void intersectLine(const Vec4 p1, const Vec4 p2,
                     std::vector<index_t>& overlaps);

  // find all overlaps with a ray
  void raycast(const Vec4 origin, const Vec4 direction,
               std::vector<index_t>& overlaps);

  // return a quality metric for this tree
  float quality() const { return _quality(_root); }

 protected:
  // bubble up tree recalculating aabbs
  void _recalc_aabbs(index_t);

  // return a quality metric for this subtree
  float _quality(index_t) const;

  // walk up the tree recalculating the aabb
  void _touched_aabb(index_t i);

  // optimize this subtree
  void _optimize(index_t i);

  // optimize the children of this node
  void _optimize(Bvh_Node& node);

  // sanity checks for the tree
  void _validate(index_t index);

  // insert node into the tree
  void _insert(index_t node);

  // find the best sibling leaf node for a given aabb
  index_t _find_best_sibling(const AABB& aabb) const;

  // insert 'node' into 'leaf'
  index_t _insert_into_leaf(index_t leaf, index_t node);

  // unlink this node from the tree but dont add it to the free list
  void _unlink(index_t index);

  // return true if a node is a leaf
  bool _is_leaf(index_t index) const;

  // access a node by index
  Bvh_Node& _get(index_t index) {
    assert(index != invalid_index);
    return _nodes[index];
  }

  // access a node by index
  const Bvh_Node& _get(index_t index) const {
    assert(index != invalid_index);
    return _nodes[index];
  }

  // get a child
  Bvh_Node& _child(index_t index, int32_t child) {
    return _get(_get(index).child[child]);
  }

  // release all nodes
  void _free_all();

  // allocate a new node from the free list
  index_t _new_node();

  // add a node to the free list
  void _free_node(index_t index);

  static const uint32_t _max_nodes = 1024 * 8;

  // free and taken bvh nodes
  std::array<Bvh_Node, _max_nodes> _nodes;
  // start index of the free list
  index_t _free_list;
  // root node of the bvh
  index_t _root;
};

}  // namespace bvh
