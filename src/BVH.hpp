#ifndef SRE_BVH_HPP
#define SRE_BVH_HPP

#include "AABB.hpp"
#include "Hittable.hpp"
#include "Triangle.hpp"

class BVHNode : Hittable {
 private:
  Hittable *left, *right;
  AABB aabb;

 public:
  BVHNode(Hittable *object) : left(object), right(object) {}
  BVHNode(const std::vector<Hittable*>& objects, int low, int high) {
    assert(!objects.empty() && 0 <= low && low < high && high <= object.size());

    sort(objects.begin(), objects.end(), zCmp);
    if (objects.size() == 1) {
      left = objects[0];
      right = objects[0];
      // TODO: add AABB method which allows Hittable* construction 
    } else {
      int mid = low + (high - low) / 2;
      left = new BVHNode(objects, low, mid);
      right = new BVHNode(objects, mid, high);
      aabb = AABB::getSurroundingAABB(left->getAABB(), right->getAABB());
    }

  }

 private:
  // TODO: cmp for BVHNode
  static bool xCmp(Hittable *left, Hittable *right) {
    AABB laabb = left->getAABB();
    AABB raabb = right->getAABB();

    return laabb.getMin().x < raabb.getMax().x;
  }

  static bool yCmp(Hittable *left, Hittable *right) {
    AABB laabb = left->getAABB();
    AABB raabb = right->getAABB();

    return laabb.getMin().y < raabb.getMax().y;
  }

  static bool zCmp(Hittable *left, Hittable *right) {
    AABB laabb = left->getAABB();
    AABB raabb = right->getAABB();

    return laabb.getMin().z < raabb.getMax().z;
  }

 public:
  // getter.
  AABB getAABB() const {return aabb;}
  
 public:
  virtual void hit(const Ray& ray, HitResult& res) const override {
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
