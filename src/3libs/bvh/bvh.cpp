#include <assert.h>
#include <3libs/bvh/bvh.h>

// enable to validate the tree after every operation
#define VALIDATE 0

namespace {
// fixed size binary heap implementation
template <typename type_t, size_t c_size>

struct bin_heap_t {
  // index of the root node
  // to simplify the implementation, heap_ is base 1 (index 0 unused).
  static const size_t c_root = 1;

  bin_heap_t() : index_(c_root) {}

  // pop the current best node in the heap (root node)
  type_t pop() {
    assert(!empty());
    // save top most value for final return
    type_t out = heap_[c_root];
    // swap last and first and shrink by one
    index_ -= 1;
    std::swap(heap_[c_root], heap_[index_]);
    // push down to correct position
    bubble_down(c_root);
    return out;
  }

  // push a new node into the heap and make rebalanced tree
  void push(type_t node) {
    assert(!full());
    // insert node into end of list
    size_t i = index_++;
    heap_[i] = node;
    bubble_up(i);
  }

  // return true if the heap is empty
  bool empty() const { return index_ <= 1; }

  // return true if the heap is full
  bool full() const { return index_ > c_size; }

  // number of nodes currently in the heap
  size_t size() const { return index_ - 1; }

  // wipe the heap back to its empty state
  void clear() { index_ = c_root; }

  // test that we have a valid binary heap
  void validate() const {
    for (size_t i = 2; i < index_; ++i) {
      assert(compare(heap_[parent(i)], heap_[i]));
    }
  }

  // discard a number of items from the bottom of the heap
  void discard(size_t num) {
    assert(index_ - num >= c_root);
    index_ -= num;
  }

 protected:
  std::array<type_t, c_size + 1> heap_;
  size_t index_;

  // check an index points to a valid node
  bool valid(size_t i) const { return i < index_; }

  // compare two nodes
  static bool inline compare(const type_t& a, const type_t& b) { return a < b; }

  // bubble an item down to its correct place in the tree
  inline void bubble_down(size_t i) {
    // while we are not at a leaf
    while (valid(child(i, 0))) {
      // get both children
      const size_t x = child(i, 0);
      const size_t y = child(i, 1);
      // select best child
      const bool select = valid(y) && compare(heap_[y], heap_[x]);
      type_t& best = select ? heap_[y] : heap_[x];
      // quit if children are not better
      if (!compare(best, heap_[i])) break;
      // swap current and child
      std::swap(best, heap_[i]);
      // repeat from child node
      i = select ? y : x;
    }
  }

  // bubble and item up to its correct place in the tree
  inline void bubble_up(size_t i) {
    // while not at the root node
    while (i > c_root) {
      const size_t j = parent(i);
      // if node is better then parent
      if (!compare(heap_[i], heap_[j])) break;
      std::swap(heap_[i], heap_[j]);
      i = j;
    }
  }

  // given an index, return the parent index
  static constexpr inline size_t parent(size_t index) { return index / 2; }

  // given an index, return one of the two child nodes (branch 0/1)
  static constexpr inline size_t child(size_t index, uint32_t branch) {
    assert(!(branch & ~1u));
    return index * 2 + branch;
  }
};

}  // namespace

namespace bvh {

bool AABB::raycast(float x0, float y0, float x1, float y1) const {
  return raycast(x0, y0, x1, y1, *this);
}

bool AABB::raycast(const Vec4 origin, const Vec4 direction) const {
  return raycast(origin, direction, *this);
}

bool AABB::intersectLine(const Vec4 p1, const Vec4 p2) const {
  return intersectLine(p1, p2, *this);
}

// 2D line segment aabb intersection test
bool AABB::raycast(float ax, float ay, float bx, float by,
                   const bvh::AABB& aabb) const {
  static const float EPSILON = 0.0001f;
  const float dx = (bx - ax) * .5f;
  const float dy = (by - ay) * .5f;
  const float ex = (aabb.maxx - aabb.minx) * .5f;
  const float ey = (aabb.maxy - aabb.miny) * .5f;
  const float cx = ax + dx - (aabb.minx + aabb.maxx) * .5f;
  const float cy = ay + dy - (aabb.miny + aabb.maxy) * .5f;
  const float adx = fabsf(dx);
  const float ady = fabsf(dy);

  if (fabsf(cx) > (ex + adx)) return false;
  if (fabsf(cy) > (ey + ady)) return false;
  return (fabsf(dx * cy - dy * cx) <= (ex * ady + ey * adx + EPSILON));
}

// 3D line segment aabb intersection test
bool AABB::intersectLine(const Vec4 p1, const Vec4 p2,
                         const bvh::AABB& aabb) const {
  static const float EPSILON = 0.0001f;
  const Vec4 min = Vec4(aabb.minx, aabb.miny, aabb.minz);
  const Vec4 max = Vec4(aabb.maxx, aabb.maxy, aabb.maxz);
  const Vec4 d = (p2 - p1) * 0.5f;
  const Vec4 e = (max - min) * 0.5f;
  const Vec4 c = p1 + d - (min + max) * 0.5f;
  const Vec4 ad = Vec4(fabsf(d.x), fabsf(d.y), fabsf(d.z));

  // Returns same vector with all components positive
  if (fabsf(c.x) > e.x + ad.x) return false;
  if (fabsf(c.y) > e.y + ad.y) return false;
  if (fabsf(c.z) > e.z + ad.z) return false;

  if (fabsf(d.y * c.z - d.z * c.y) > e.y * ad.z + e.z * ad.y + EPSILON)
    return false;
  if (fabsf(d.z * c.x - d.x * c.z) > e.z * ad.x + e.x * ad.z + EPSILON)
    return false;
  if (fabsf(d.x * c.y - d.y * c.x) > e.x * ad.y + e.y * ad.x + EPSILON)
    return false;
  return true;
}

// TODO: update intersction point result to Entity data
// line segment aabb intersection test
bool AABB::raycast(const Vec4 origin, const Vec4 direction,
                   const bvh::AABB& aabb) const {
  Ray ray = Ray(origin, direction);

  const Vec4 min = Vec4(aabb.minx, aabb.miny, aabb.minz);
  const Vec4 max = Vec4(aabb.maxx, aabb.maxy, aabb.maxz);

  return ray.intersectBox(min, max);
}

AABBTree::AABBTree()
    : growth(16.f), _free_list(invalid_index), _root(invalid_index) {
  clear();
}

AABBTree::~AABBTree() {
  TYRA_LOG("Destroying AABBTree...");
  clear();
}

void AABBTree::clear() {
  TYRA_LOG("Clearing...");
  _free_all();
  _root = invalid_index;
}

bool AABBTree::_is_leaf(index_t index) const { return get(index).is_leaf(); }

// return a quality metric for this subtree
float AABBTree::_quality(index_t n) const {
  if (n == invalid_index) {
    return 0.f;
  }
  const Bvh_Node& node = _get(n);
  if (node.is_leaf()) {
    return 0.f;
  }
  // cost metric is the sum of the surface area of all interior nodes (non root
  // or leaf)
  return ((n == _root) ? 0.f : node.aabb.area()) + _quality(node.child[0]) +
         _quality(node.child[1]);
}

index_t AABBTree::_insert_into_leaf(index_t leaf, index_t node) {
  assert(_is_leaf(leaf));
  // create new internal node
  index_t inter = _new_node();
  // insert children
  _get(inter).child[0] = leaf;
  _get(inter).child[1] = node;
  // keep track of the parents
  _get(inter).parent = invalid_index;  // fixed up by callee
  _get(leaf).parent = inter;
  _get(node).parent = inter;
  // recalculate the aabb on way up
  _get(inter).aabb = AABB::find_union(_get(leaf).aabb, _get(node).aabb);
  // new child is the intermediate node
  return inter;
}

index_t AABBTree::insert(AABB aabb, void* user_data) {
  // create the new node
  index_t index = _new_node();
  assert(index != invalid_index);
  auto& node = _get(index);
  // grow the aabb by a factor
  node.aabb = AABB::grow(aabb, growth);
  //
  node.user_data = user_data;
  node.parent = invalid_index;
  // mark that this is a leaf
  node.child[0] = invalid_index;
  node.child[1] = invalid_index;
  // insert into the tree
  if (_root == invalid_index) {
    _root = index;
    return index;
  }
  if (_is_leaf(_root)) {
    _root = _insert_into_leaf(_root, index);
    _validate(_root);
    return index;
  }
  _insert(index);
#if VALIDATE
  _validate(_root);
#endif
  // return this index
  return index;
}

void AABBTree::_recalc_aabbs(index_t i) {
  // walk from leaf to root recalculating aabbs
  while (i != invalid_index) {
    Bvh_Node& y = _get(i);
    y.aabb = AABB::find_union(_get(y.child[0]).aabb, _get(y.child[1]).aabb);
    _optimize(y);
    i = y.parent;
  }
}

index_t AABBTree::_find_best_sibling(const AABB& aabb) const {
  struct search_t {
    index_t index;
    float cost;

    bool operator<(const search_t& rhs) const { return cost < rhs.cost; }
  };
  // XXX: warning, fixed size here
  bin_heap_t<search_t, 1024> pqueue;

  if (_root != invalid_index) {
    pqueue.push(search_t{_root, 0.f});
  }

  float best_cost = INFINITY;
  index_t best_index = invalid_index;

  while (!pqueue.empty()) {
    // pop one node (lowest cost so far)
    const search_t s = pqueue.pop();
    const Bvh_Node& n = _get(s.index);
    // find the cost for inserting into this node
    const AABB uni = AABB::find_union(aabb, n.aabb);
    const float growth = uni.area() - n.aabb.area();
    assert(growth >= 0.f);
    const float cost = s.cost + growth;
    // skip the entire subtree if we know we have a better choice
    if (cost >= best_cost) {
      continue;
    }
    if (n.is_leaf()) {
      best_cost = cost;
      best_index = s.index;
    } else {
      assert(n.child[0] != invalid_index);
      assert(n.child[1] != invalid_index);
      pqueue.push(search_t{n.child[0], cost});
      pqueue.push(search_t{n.child[1], cost});
    }
  }
  // return the best leaf we cound find
  return best_index;
}

void AABBTree::_insert(index_t node) {
  // note: this insert phase assumes that there are at least two nodes already
  //       in the tree and thus _root is a non leaf node.

  // find the best sibling for node
  const AABB& aabb = _get(node).aabb;
  index_t sibi = _find_best_sibling(aabb);
  assert(sibi != invalid_index);
  // once the best leaf has been found we come to the insertion phase
  const Bvh_Node& sib = _get(sibi);
  assert(sib.is_leaf());
  const index_t parent = sib.parent;
  const index_t inter = _insert_into_leaf(sibi, node);
  _get(inter).parent = parent;
  // fix up parent child relationship
  if (parent != invalid_index) {
    Bvh_Node& p = _get(parent);
    p.replace_child(sibi, inter);
  }
  // recalculate aabb and optimize on the way up
  _recalc_aabbs(parent);
}

void AABBTree::remove(index_t index) {
  assert(index != invalid_index);
  assert(_is_leaf(index));
  auto& node = _get(index);
  _unlink(index);
  node.child[0] = _free_list;
  node.child[1] = invalid_index;
  _free_list = index;
#if VALIDATE
  _validate(_root);
#endif
}

void AABBTree::move(index_t index, const AABB& aabb) {
  assert(index != invalid_index);
  assert(_is_leaf(index));
  auto& node = _get(index);
  // check fat aabb against slim new aabb for hysteresis on our updates
  if (node.aabb.contains(aabb)) {
    // this is okay and we can early exit
    return;
  }
  // effectively remove this node from the tree
  _unlink(index);
  // save the fat version of this aabb
  node.aabb = AABB::grow(aabb, growth);
  // insert into the tree
  if (_root == invalid_index) {
    _root = index;
    return;
  }
  if (_is_leaf(_root)) {
    _root = _insert_into_leaf(_root, index);
  }
  _insert(index);
#if VALIDATE
  _validate(_root);
#endif
}

void AABBTree::_free_all() {
  TYRA_LOG("freeing all...");
  _free_list = 0;
  for (uint32_t i = 0; i < _max_nodes; ++i) {
    _get(i).child[0] = i + 1;
    _get(i).child[1] = invalid_index;
    _get(i).parent = invalid_index;
  }
  // mark the end of the list of free nodes
  _get(_max_nodes - 1).child[0] = invalid_index;
  _get(_max_nodes - 1).child[1] = invalid_index;
  _get(_max_nodes - 1).parent = invalid_index;
  _root = invalid_index;
}

index_t AABBTree::_new_node() {
  index_t out = _free_list;
  assert(out != invalid_index);
  _free_list = get(_free_list).child[0];
  return out;
}

void AABBTree::_unlink(index_t index) {
  // assume we only ever unlink a leaf
  assert(_is_leaf(index));
  auto& node = _get(index);

  // special case root node
  if (index == _root) {
    assert(node.parent == invalid_index);
    // case 1 (node was root)
    _root = invalid_index;
    return;
  }

  auto& p0 = _get(node.parent);

  if (p0.parent == invalid_index) {
    // case 2 (p0 is root)
    //       P0
    //  node     ?

    // find the sibling node
    index_t sib = (p0.child[0] == index) ? 1 : 0;
    // promote as the new root
    _root = p0.child[sib];
    _get(_root).parent = invalid_index;
    // free p0 node
    _free_node(node.parent);
    node.parent = invalid_index;
    return;
  }

  auto& p1 = _get(p0.parent);

  // case 3 (p0 is not root)
  //              p1
  //       p0
  //  node     ?

  // find the configuration of p1 children
  // find child index of p0 for p1
  index_t p1c = (p1.child[0] == node.parent) ? 0 : 1;
  assert(p1.child[p1c] == node.parent);

  // find the configuration of p0 children
  // find child index of 'node' for p0
  index_t p0c = (p0.child[0] == index) ? 0 : 1;
  assert(p0.child[p0c] == index);

  // promote sibling of 'node' into place of p0
  // find sibling of 'node'
  index_t sib = p0.child[p0c ^ 1];
  assert(sib != invalid_index);
  // promote sibling of 'node' into place of p0
  p1.child[p1c] = sib;
  _get(sib).parent = p0.parent;

  // recalculate the remaining tree aabbs, p1 up
  _touched_aabb(p0.parent);

  // free p0
  _free_node(node.parent);
  // node is now unlinked
  node.parent = invalid_index;
}

void AABBTree::_touched_aabb(index_t i) {
  while (i != invalid_index) {
    Bvh_Node& node = _get(i);
    assert(!node.is_leaf());
    assert(node.child[0] != invalid_index);
    assert(node.child[1] != invalid_index);
    node.aabb = AABB::find_union(_child(i, 0).aabb, _child(i, 1).aabb);
    i = node.parent;
  }
}

void AABBTree::_free_node(index_t index) {
  assert(index != invalid_index);
  auto& node = _get(index);
  node.child[0] = _free_list;
  _free_list = index;
}

void AABBTree::_validate(index_t index) {
  if (index == invalid_index) {
    return;
  }

  auto& node = _get(index);

  if (index == _root) {
    assert(_get(index).parent == invalid_index);
  }
  // check parents children
  if (node.parent != invalid_index) {
    auto& parent = _get(node.parent);
    assert(parent.child[0] != parent.child[1]);
    assert(parent.child[0] == index || parent.child[1] == index);
  }
  if (_is_leaf(index)) {
    // leaves should have no children
    assert(node.child[0] == invalid_index);
    assert(node.child[1] == invalid_index);
  } else {
    // interior nodes should have two children
    assert(node.child[0] != invalid_index);
    assert(node.child[1] != invalid_index);
    // validate childrens parent indices
    assert(_child(index, 0).parent == index);
    assert(_child(index, 1).parent == index);
    // validate aabbs
    assert(node.aabb.contains(_child(index, 0).aabb));
    assert(node.aabb.contains(_child(index, 1).aabb));
    // validate child nodes
    _validate(node.child[0]);
    _validate(node.child[1]);

    // XXX: check area of aabb is minimal?
  }
}

void AABBTree::_optimize(Bvh_Node& node) {
#if VALIDATE
  const float before = quality();
#endif

  // four possible rotations
  //
  //        n                   n
  //   c0       c1         c0      *x0
  // x0  x1             *c1  x1
  //
  //        n                   n
  //   c0       c1         c0      *x1
  // x0  x1              x0 *c1
  //
  // same as above but with c0 and c1 roles swapped

  for (int i = 0; i < 2; ++i) {
    // gen 1
    const auto c0i = node.child[0];
    const auto c1i = node.child[1];
    if (c0i == invalid_index || c1i == invalid_index) {
      return;
    }
    auto& c0 = _get(c0i);
    auto& c1 = _get(c1i);

    // gen 2
    const auto x0i = c0.child[0];
    const auto x1i = c0.child[1];
    if (x0i == invalid_index || x1i == invalid_index) {
      // swap c0 and c1 and try again (rotations 3 and 4)
      std::swap(node.child[0], node.child[1]);
      continue;
    }
    auto& x0 = _get(x0i);
    auto& x1 = _get(x1i);

    // note: this heuristic only looks at minimizing the area of c0
    //       which is all that is affected by these rotations

    // current
    const float h0 = c0.aabb.area();
    // rotation 1
    const AABB a1 = AABB::find_union(c1.aabb, x1.aabb);
    const float h1 = a1.area();
    // rotation 2
    const AABB a2 = AABB::find_union(x0.aabb, c1.aabb);
    const float h2 = a2.area();

    if (h1 < h2) {
      if (h1 < h0) {
        // do rotation 1 (swap x0/c1)
        //        n                   n
        //   c0       c1  ->     c0      *x0
        // x0  x1             *c1  x1
        //
        c1.parent = c0i;
        x0.parent = c0.parent;
        std::swap(c0.child[0], node.child[1]);  // x0 and c1
        c0.aabb = a1;
        assert(c0.aabb.contains(c1.aabb));
        assert(c0.aabb.contains(x1.aabb));
      }
    } else {
      if (h2 < h0) {
        // do rotation 2 (swap x1/c1)
        //        n                   n
        //   c0       c1  ->     c0      *x1
        // x0  x1              x0 *c1
        //
        c1.parent = c0i;
        x1.parent = c0.parent;
        std::swap(c0.child[1], node.child[1]);  // x1 and c1
        c0.aabb = a2;
        assert(c0.aabb.contains(x0.aabb));
        assert(c0.aabb.contains(c1.aabb));
      }
    }
    // swap c0 and c1 and try again (rotations 3 and 4)
    std::swap(node.child[0], node.child[1]);
  }

#if VALIDATE
  const float after = quality();
  assert(after - 1.f <= before);
#endif
}

void AABBTree::find_overlaps(const AABB& bb, std::vector<index_t>& overlaps) {
  std::vector<index_t> stack;
  stack.reserve(128);
  if (_root != invalid_index) {
    stack.push_back(_root);
  }
  while (!stack.empty()) {
    // pop one node
    const index_t ni = stack.back();
    assert(ni != invalid_index);
    const Bvh_Node& n = _get(ni);

    // if these aabbs overlap
    if (AABB::overlaps(bb, n.aabb)) {
      if (n.is_leaf()) {
        overlaps.push_back(ni);
      } else {
        assert(n.child[0] != invalid_index);
        assert(n.child[1] != invalid_index);
        stack.back() = n.child[0];
        stack.push_back(n.child[1]);
        continue;
      }
    }
    // pop node from the stack
    stack.resize(stack.size() - 1);
  }
}

void AABBTree::find_overlaps(index_t node, std::vector<index_t>& overlaps) {
  const Bvh_Node& n = _get(node);
  find_overlaps(n.aabb, overlaps);
}

void AABBTree::raycast(float x0, float y0, float x1, float y1,
                       std::vector<index_t>& overlaps) {
  std::vector<index_t> stack;
  stack.reserve(128);
  if (_root != invalid_index) {
    stack.push_back(_root);
  }
  while (!stack.empty()) {
    // pop one node
    const index_t ni = stack.back();
    assert(ni != invalid_index);
    const Bvh_Node& n = _get(ni);
    // if the ray and aabb overlap
    if (n.aabb.raycast(x0, y0, x1, y1, n.aabb)) {
      if (n.is_leaf()) {
        overlaps.push_back(ni);
      } else {
        assert(n.child[0] != invalid_index);
        assert(n.child[1] != invalid_index);
        stack.back() = n.child[0];
        stack.push_back(n.child[1]);
        continue;
      }
    }
    // pop node
    stack.resize(stack.size() - 1);
  }
}

void AABBTree::raycast(const Vec4 origin, const Vec4 direction,
                       std::vector<index_t>& overlaps) {
  std::vector<index_t> stack;
  stack.reserve(128);
  if (_root != invalid_index) {
    stack.push_back(_root);
  }
  while (!stack.empty()) {
    // pop one node
    const index_t ni = stack.back();
    assert(ni != invalid_index);
    const Bvh_Node& n = _get(ni);
    // if the ray and aabb overlap
    if (n.aabb.raycast(origin, direction, n.aabb)) {
      if (n.is_leaf()) {
        overlaps.push_back(ni);
      } else {
        assert(n.child[0] != invalid_index);
        assert(n.child[1] != invalid_index);
        stack.back() = n.child[0];
        stack.push_back(n.child[1]);
        continue;
      }
    }
    // pop node
    stack.resize(stack.size() - 1);
  }
}

void AABBTree::intersectLine(const Vec4 p1, const Vec4 p2,
                             std::vector<index_t>& overlaps) {
  std::vector<index_t> stack;
  stack.reserve(128);
  if (_root != invalid_index) {
    stack.push_back(_root);
  }
  while (!stack.empty()) {
    // pop one node
    const index_t ni = stack.back();
    assert(ni != invalid_index);
    const Bvh_Node& n = _get(ni);
    // if the ray and aabb overlap
    if (n.aabb.intersectLine(p1, p2, n.aabb)) {
      if (n.is_leaf()) {
        overlaps.push_back(ni);
      } else {
        assert(n.child[0] != invalid_index);
        assert(n.child[1] != invalid_index);
        stack.back() = n.child[0];
        stack.push_back(n.child[1]);
        continue;
      }
    }
    // pop node
    stack.resize(stack.size() - 1);
  }
}

}  // namespace bvh
