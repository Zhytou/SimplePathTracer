#ifndef SRE_AABB_HPP
#define SRE_AABB_HPP

#include "Hittable.hpp"
#include "Triangle.hpp"
#include "Vec.hpp"

class AABB : Hittable {
 private:
  Vec3<float> minXYZ, maxXYZ;

 public:
  AABB(const Vec3<float>& a, const Vec3<float>& b) : minXYZ(a), maxXYZ(b) {}
  AABB(const Triangle& triangle) {
    minXYZ.x = std::min(triangle.p1.x, std::min(triangle.p2.x, triangle.p3.x));
    minXYZ.y = std::min(triangle.p1.y, std::min(triangle.p2.y, triangle.p3.y));
    minXYZ.z = std::min(triangle.p1.z, std::min(triangle.p2.z, triangle.p3.z));

    maxXYZ.x = std::max(triangle.p1.x, std::max(triangle.p2.x, triangle.p3.x));
    maxXYZ.y = std::max(triangle.p1.y, std::max(triangle.p2.y, triangle.p3.y));
    maxXYZ.z = std::max(triangle.p1.z, std::max(triangle.p2.z, triangle.p3.z));
  }

 public:
  // getter.
  Vec3<float> getMin() const { return minXYZ; }
  Vec3<float> getMax() const { return maxXYZ; }

  static AABB getSurroundingAABB(const AABB& child1, const AABB& child2) {
    Vec3<float> minXYZ, maxXYZ;

    minXYZ.x = std::min(child1.getMin().x, child2.getMin().x);
    minXYZ.y = std::min(child1.getMin().y, child2.getMin().y);
    minXYZ.z = std::min(child1.getMin().z, child2.getMin().z);

    maxXYZ.x = std::max(child1.getMax().x, child2.getMax().x);
    maxXYZ.y = std::max(child1.getMax().y, child2.getMax().y);
    maxXYZ.z = std::max(child1.getMax().z, child2.getMax().z);

    return AABB(minXYZ, maxXYZ);
  }

 public:
  virtual void hit(const Ray& ray, HitResult& res) const override {}
};

#endif