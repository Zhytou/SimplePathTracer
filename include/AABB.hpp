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
  AABB();
  AABB(const Vec3<float>& a, const Vec3<float>& b);
  AABB(const Hittable* object);

 public:
  // getter.
  virtual Vec3<float> getMinXYZ() const override;
  virtual Vec3<float> getMaxXYZ() const override;
  static AABB getSurroundingAABB(const AABB& child1, const AABB& child2);

  // print.
  virtual void printStatus() const override;

 public:
  virtual void hit(const Ray& ray, HitResult& res) const override;
};
}  // namespace sre

#endif