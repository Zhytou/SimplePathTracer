#ifndef SRE_BVH_HPP
#define SRE_BVH_HPP

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "AABB.hpp"
#include "Hittable.hpp"
#include "Triangle.hpp"

namespace sre {
class BVHNode;

typedef BVHNode BVH;

class BVHNode : public Hittable {
 private:
  Hittable *left, *right;
  AABB aabb;
  int nodeNum;

 public:
  BVHNode(Hittable *object);
  BVHNode(std::vector<Hittable *> &objects, int low, int high);
  ~BVHNode();

 public:
  static bool xCmp(Hittable *left, Hittable *right);
  static bool yCmp(Hittable *left, Hittable *right);
  static bool zCmp(Hittable *left, Hittable *right);

 public:
  // getter.
  virtual Vec3<float> getMinXYZ() const override;
  virtual Vec3<float> getMaxXYZ() const override;
  AABB getAABB() const;
  int getNodeNum() const;

  // print.
  virtual void printStatus() const override;

 public:
  virtual void hit(const Ray &ray, HitResult &res) const override;
};

}  // namespace sre

#endif
