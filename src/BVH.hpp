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
  BVHNode(const std::vector<Triangle>& triangles) {}

 private:
  // TODO: cmp for BVHNode

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