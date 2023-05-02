#include "entities/collision/aabb_tree.hpp"

AABBTree::AABBTree() {}

AABBTree::~AABBTree() {}

void AABBTree::Add(AABB* aabb) {
  if (m_root) {
    // not first node, insert node to tree
    Node* node = new Node();
    node->SetLeaf(aabb);
    node->UpdateAABB(m_margin);
    InsertNode(node, &m_root);
  } else {
    // first node, make root
    m_root = new Node();
    m_root->SetLeaf(aabb);
    m_root->UpdateAABB(m_margin);
  }
}

void AABBTree::Remove(AABB* aabb) {
  Node* node = static_cast<Node*>(aabb->userData);

  // remove two-way link
  node->data = nullptr;
  aabb->userData = nullptr;

  RemoveNode(node);
}

void AABBTree::Update() {
  if (m_root) {
    if (m_root->IsLeaf())
      m_root->UpdateAABB(m_margin);
    else {
      // grab all invalid nodes
      m_invalidNodes.clear();
      UpdateNodeHelper(m_root, m_invalidNodes);

      // re-insert all invalid nodes
      for (Node* node : m_invalidNodes) {
        // grab parent link
        // (pointer to the pointer that points to parent)
        Node* parent = node->parent;
        Node* sibling = node->GetSibling();
        Node** parentLink = parent->parent
                                ? (parent == parent->parent->children[0]
                                       ? &parent->parent->children[0]
                                       : &parent->parent->children[1])
                                : &m_root;

        // replace parent with sibling
        sibling->parent =
            parent->parent ? parent->parent : nullptr;  // root has null parent

        *parentLink = sibling;
        delete parent;

        // re-insert node
        node->UpdateAABB(m_margin);
        InsertNode(node, &m_root);
      }
      m_invalidNodes.clear();
    }
  }

  void AABBTree::UpdateNodeHelper(Node * node, NodeList & invalidNodes) {
    if (node->IsLeaf()) {
      // check if fat AABB doesn't
      // contain the collider's AABB anymore
      if (!node->aabb.Contains(node->data->aabb)) invalidNodes.push_back(node);
    } else {
      UpdateNodeHelper(node->children[0], invalidNodes);
      UpdateNodeHelper(node->children[1], invalidNodes);
    }
  }
}

void AABBTree::InsertNode(Node* node, Node** parent) {
  Node* p = *parent;
  if (p->IsLeaf()) {
    // parent is leaf, simply split
    Node* newParent = new Node();
    newParent->parent = p->parent;
    newParent->SetBranch(node, p);
    *parent = newParent;
  } else {
    // parent is branch, compute volume differences
    // between pre-insert and post-insert
    const AABB* aabb0 = p->children[0]->aabb;
    const AABB* aabb1 = p->children[1]->aabb;
    const float volumeDiff0 =
        aabb0->Union(node->aabb).Volume() - aabb0->Volume();
    const float volumeDiff1 =
        aabb1->Union(node->aabb).Volume() - aabb1->Volume();

    // insert to the child that gives less volume increase
    if (volumeDiff0 < volumeDiff1)
      InsertNode(node, &p->children[0]);
    else
      InsertNode(node, &p->children[1]);
  }

  // update parent AABB
  // (propagates back up the recursion stack)
  (*parent)->UpdateAABB(m_margin);
}

void AABBTree::RemoveNode(Node* node) {
  // replace parent with sibling, remove parent node
  Node* parent = node->parent;
  if (parent)  // node is not root
  {
    Node* sibling = node->GetSibling();
    if (parent->parent)  // if there's a grandparent
    {
      // update links
      sibling->parent = parent->parent;
      (parent == parent->parent->children[0] ? parent->parent->children[0]
                                             : parent->parent->children[1]) =
          sibling;
    } else  // no grandparent
    {
      // make sibling root
      Node* sibling = node->GetSibling();
      m_root = sibling;
      sibling->parent = nullptr;
    }
    delete node;
    delete parent;
  } else  // node is root
  {
    m_root = nullptr;
    delete node;
  }
}

// ColliderPairList& AABBTree::ComputePairs(void) {
//   m_pairs.clear();

//   // early out
//   if (!m_root || m_root->IsLeaf()) return m_pairs;

//   // clear Node::childrenCrossed flags
//   ClearChildrenCrossFlagHelper(m_root);

//   // base recursive call
//   ComputePairsHelper(m_root->children[0], m_root->children[1]);

//   return m_pairs;
// }

// void AABBTree::ClearChildrenCrossFlagHelper(Node* node) {
//   node->childrenCrossed = false;
//   if (!node->IsLeaf()) {
//     ClearChildrenCrossFlagHelper(node->children[0]);
//     ClearChildrenCrossFlagHelper(node->children[1]);
//   }
// }

// void AABBTree::CrossChildren(Node* node) {
//   if (!node->childrenCrossed) {
//     ComputePairsHelper(node->children[0], node->children[1]);
//     node->childrenCrossed = true;
//   }
// }

// void AABBTree::ComputePairsHelper(Node* n0, Node* n1) {
//   if (n0->IsLeaf()) {
//     // 2 leaves, check proxies instead of fat AABBs
//     if (n1->IsLeaf()) {
//       if (n0->data->Collides(*n1->data))
//         m_pairs.push_back(
//             AllocatePair(n0->data->Collider(), n1->data->Collider()));
//     }
//     // 1 branch / 1 leaf, 2 cross checks
//     else {
//       CrossChildren(n1);
//       ComputePairsHelper(n0, n1->children[0]);
//       ComputePairsHelper(n0, n1->children[1]);
//     }
//   } else {
//     // 1 branch / 1 leaf, 2 cross checks
//     if (n1->IsLeaf()) {
//       CrossChildren(n0);
//       ComputePairsHelper(n0->children[0], n1);
//       ComputePairsHelper(n0->children[1], n1);
//     }
//     // 2 branches, 4 cross checks
//     else {
//       CrossChildren(n0);
//       CrossChildren(n1);
//       ComputePairsHelper(n0->children[0], n1->children[0]);
//       ComputePairsHelper(n0->children[0], n1->children[1]);
//       ComputePairsHelper(n0->children[1], n1->children[0]);
//       ComputePairsHelper(n0->children[1], n1->children[1]);
//     }
//   }  // end of if (n0->IsLeaf())
// }

// void AABBTree::Pick(const Vec3& pos, PickResult& result) {
//   std::queue<Node*> q;
//   if (m_root) q.push(m_root);

//   while (!q.empty()) {
//     Node& node = *q.front();
//     q.pop();

//     if (node.IsLeaf()) {
//       if (node.data->Collides(pos)) result.push_back(node.data->Collider());
//     } else {
//       q.push(node.children[0]);
//       q.push(node.children[1]);
//     }
//   }
// }

// const RayCastResult AABBTree::RayCast(const Ray3& ray, float maxDistance) {
//   RayCastResult result;
//   result.hit = false;
//   result.t = 1.0f;

//   std::queue<Node*> q;
//   if (m_root) {
//     q.push(m_root);
//   }

//   while (!q.empty()) {
//     Node& node = *q.front();
//     q.pop();

//     AABB& colliderAABB = *node.data;
//     AABB& aabb = node.IsLeaf() ? colliderAABB : node.aabb;

//     float t;
//     if (RayAABB(ray, aabb, maxDistance, t)) {
//       // the node cannot possibly give closer results, skip
//       if (result.hit && result.t < t) continue;

//       if (node.IsLeaf()) {
//         Collider& collider = *colliderAABB.Collider();
//         Vec3 n;
//         float t;
//         if (collider.RayCast(ray, maxDistance, t, n)) {
//           if (result.hit)  // compare hit
//           {
//             if (t < result.t) {
//               result.collider = &collider;
//               result.t = t;
//               result.normal = n;
//               result.intersection =
//                   ray.pos + t * maxDistance * ray.dir.Normalized();
//             }
//           } else  // first hit
//           {
//             result.hit = true;
//             result.collider = &collider;
//             result.t = t;
//             result.ray = ray;
//             result.normal = n;
//             result.intersection =
//                 ray.pos + t * maxDistance * ray.dir.Normalized();
//           }
//         }
//       } else  // is branch
//       {
//         q.push(node.children[0]);
//         q.push(node.children[1]);
//       }
//     }
//   }

//   return result;
// }
