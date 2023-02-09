#ifndef SRE_BVH_HPP
#define SRE_BVH_HPP

#include <algorithm>
#include <cassert>

#include "AABB.hpp"
#include "Hittable.hpp"
#include "Triangle.hpp"

class BVHNode;

typedef BVHNode BVH;

class BVHNode : public Hittable {
 private:
  Hittable *left, *right;
  AABB aabb;
  int nodeNum;

 public:
  BVHNode(Hittable *object)
      : left(object), right(object), aabb(object), nodeNum(1) {}
  BVHNode(const std::vector<Hittable *> &objects, int low, int high)
      : nodeNum(high - low) {
    assert(!objects.empty() && 0 <= low && low < high &&
           high <= objects.size());

    if (high - low == 1) {
      left = objects[0];
      right = objects[0];
      aabb = AABB(objects[0]);
    } else {
      int mid = low + (high - low) / 2;
      left = new BVHNode(objects, low, mid);
      right = new BVHNode(objects, mid, high);
      aabb = AABB::getSurroundingAABB(AABB(left), AABB(right));
    }
  }
  ~BVHNode() {
    if (left != nullptr) {
      delete left;
    }

    if (right != nullptr) {
      delete right;
    }
  }

 public:
  static bool xCmp(Hittable *left, Hittable *right) {
    assert(left != nullptr && right != nullptr);
    AABB laabb(left);
    AABB raabb(right);

    return laabb.getMinXYZ().x < raabb.getMaxXYZ().x;
  }

  static bool yCmp(Hittable *left, Hittable *right) {
    assert(left != nullptr && right != nullptr);
    AABB laabb(left);
    AABB raabb(right);

    return laabb.getMinXYZ().y < raabb.getMaxXYZ().y;
  }

  static bool zCmp(Hittable *left, Hittable *right) {
    assert(left != nullptr && right != nullptr);

    // ? 编译为啥能通过，验证后修改xCmp和yCmp
    return left->getMinXYZ().z < right->getMaxXYZ().z;
  }

 public:
  // getter.
  virtual Vec3<float> getMinXYZ() const override { return aabb.getMinXYZ(); }
  virtual Vec3<float> getMaxXYZ() const override { return aabb.getMaxXYZ(); }
  AABB getAABB() const { return aabb; }
  int getNodeNum() const { return nodeNum; }

 public:
  virtual void hit(const Ray &ray, HitResult &res) const override {
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
    } else if (lres.isHit && !rres.isHit) {
      res = rres;
    } else {
      res.isHit = false;
    }
  }
};

#endif
