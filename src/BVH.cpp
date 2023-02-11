#include "../include/BVH.hpp"

namespace sre {
BVHNode::BVHNode(Hittable *object) {
  assert(object != nullptr);

  left = object;
  right = object;
  aabb = AABB(object);
  nodeNum = 1;
}

BVHNode::BVHNode(std::vector<Hittable *> &objects, int low, int high)
    : nodeNum(high - low) {
  assert(!objects.empty() && 0 <= low && low < high && high <= objects.size());

  switch (randInt(3)) {
    case 2:
      std::sort(objects.begin(), objects.end(), BVH::xCmp);
    case 1:
      std::sort(objects.begin(), objects.end(), BVH::yCmp);
    default:
      std::sort(objects.begin(), objects.end(), BVH::zCmp);
  }

  if (high - low == 1) {
    left = objects[low];
    right = objects[low];
    aabb = AABB(objects[low]);
  } else {
    int mid = low + (high - low) / 2;
    left = new BVHNode(objects, low, mid);
    right = new BVHNode(objects, mid, high);
    aabb = AABB::getSurroundingAABB(AABB(left), AABB(right));
  }
}

BVHNode::~BVHNode() {
  if (left != nullptr && right != nullptr) {
    if (left != right) {
      delete left;
    }
    delete right;
  } else if (left != nullptr && right == nullptr) {
    delete left;
  } else if (left == nullptr && right != nullptr) {
    delete right;
  }
}

bool BVHNode::xCmp(Hittable *left, Hittable *right) {
  assert(left != nullptr && right != nullptr);
  return left->getMinXYZ().x < right->getMinXYZ().x;
}

bool BVHNode::yCmp(Hittable *left, Hittable *right) {
  assert(left != nullptr && right != nullptr);
  return left->getMinXYZ().y < right->getMinXYZ().y;
}

bool BVHNode::zCmp(Hittable *left, Hittable *right) {
  assert(left != nullptr && right != nullptr);
  return left->getMinXYZ().z < right->getMinXYZ().z;
}

void BVHNode::hit(const Ray &ray, HitResult &res) const {
  aabb.hit(ray, res);

  if (!res.isHit) {
    return;
  }

  HitResult lres, rres;
  left->hit(ray, lres);
  right->hit(ray, rres);

  if (lres.isHit && rres.isHit) {
    if (lres.distance < rres.distance) {
      res = lres;
    } else {
      res = rres;
    }
  } else if (lres.isHit && !rres.isHit) {
    res = lres;
  } else if (!lres.isHit && rres.isHit) {
    res = rres;
  } else {
    res.isHit = false;
  }
}
}  // namespace sre
