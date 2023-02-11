#ifndef SRE_AABB_HPP
#define SRE_AABB_HPP

#include <cassert>

#include "Hittable.hpp"
#include "Vec.hpp"

namespace sre {

class AABB : public Hittable {
 private:
  Vec3<float> minXYZ, maxXYZ;

 public:
  AABB() : minXYZ(0, 0, 0), maxXYZ(0, 0, 0) {}
  AABB(const Vec3<float>& a, const Vec3<float>& b) : minXYZ(a), maxXYZ(b) {}
  AABB(const Hittable* object) {
    assert(object != nullptr);
    minXYZ = object->getMinXYZ();
    maxXYZ = object->getMaxXYZ();
  }

 public:
  // getter.
  virtual Vec3<float> getMinXYZ() const override { return minXYZ; }
  virtual Vec3<float> getMaxXYZ() const override { return maxXYZ; }
  static AABB getSurroundingAABB(const AABB& child1, const AABB& child2);

  // print.
  virtual void printStatus() const override;

 public:
  virtual void hit(const Ray& ray, HitResult& res) const override;
};
}  // namespace sre

#endif