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
  virtual void hit(const Ray& ray, HitResult& res) const override {
    Vec3<float> origin = ray.getOrigin();
    Vec3<float> direction = ray.getDirection();
    Vec3<float> tMin, tMax;

    double tx_min, ty_min, tz_min;
    double tx_max, ty_max, tz_max;

    if (abs(direction.x) < 0.000001f) {
      // 若射线方向矢量的x轴分量为0且原点不在盒体内
      if (origin.x < minXYZ.x || origin.x > maxXYZ.x) {
        res.isHit = false;
        return;
      }
    } else {
      if (direction.x >= 0) {
        tx_min = (minXYZ.x - origin.x) / direction.x;
        tx_max = (maxXYZ.x - origin.x) / direction.x;
      } else {
        tx_min = (maxXYZ.x - origin.x) / direction.x;
        tx_max = (minXYZ.x - origin.x) / direction.x;
      }
    }

    if (abs(direction.y) < 0.000001f) {
      // 若射线方向矢量的x轴分量为0且原点不在盒体内
      if (origin.y < maxXYZ.y || origin.y > minXYZ.y) {
        res.isHit = false;
        return;
      }
    } else {
      if (direction.y >= 0) {
        tMin.y = (minXYZ.y - origin.y) / direction.y;
        tMax.y = (maxXYZ.y - origin.y) / direction.y;
      } else {
        tMin.y = (maxXYZ.y - origin.y) / direction.y;
        tMax.y = (minXYZ.y - origin.y) / direction.y;
      }
    }

    if (abs(direction.z) < 0.000001f) {
      // 若射线方向矢量的x轴分量为0且原点不在盒体内
      if (origin.z < maxXYZ.z || origin.z > minXYZ.z) {
        res.isHit = false;
        return;
      }
    } else {
      if (direction.z >= 0) {
        tMin.z = (minXYZ.z - origin.z) / direction.z;
        tMax.z = (maxXYZ.z - origin.z) / direction.z;
      } else {
        tMin.z = (maxXYZ.z - origin.z) / direction.z;
        tMax.z = (minXYZ.z - origin.z) / direction.z;
      }
    }

    float t0, t1;

    // 光线进入平面处（最靠近的平面）的最大t值
    t0 = std::max(tMin.z, std::max(tMin.y, tMin.x));

    // 光线离开平面处（最远离的平面）的最小t值
    t1 = std::min(tMax.z, std::min(tMax.y, tMax.x));

    res.isHit = t0 < t1;
    return;
  }
};

#endif