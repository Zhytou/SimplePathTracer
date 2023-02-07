#ifndef SRE_BVH_HPP
#define SRE_BVH_HPP

#include "AABB.hpp"
#include "Hittable.hpp"

class BVHNode : Hittable {
 private:
  Hittable *left, *right;
  AABB aabb;

 public:
  virtual void hit(Ray& ray, HitResult& res) {}
};

#endif