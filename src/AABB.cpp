#include "../include/AABB.hpp"

namespace sre {
AABB::AABB() : minXYZ(0, 0, 0), maxXYZ(0, 0, 0) {}
AABB::AABB(const Vec3<float>& a, const Vec3<float>& b) : minXYZ(a), maxXYZ(b) {}
AABB::AABB(const Hittable* object) {
  assert(object != nullptr);
  minXYZ = object->getMinXYZ();
  maxXYZ = object->getMaxXYZ();
}

Vec3<float> AABB::getMinXYZ() const { return minXYZ; }
Vec3<float> AABB::getMaxXYZ() const { return maxXYZ; }
AABB AABB::getSurroundingAABB(const AABB& child1, const AABB& child2) {
  Vec3<float> minXYZ, maxXYZ;

  minXYZ.x = std::min(child1.getMinXYZ().x, child2.getMinXYZ().x);
  minXYZ.y = std::min(child1.getMinXYZ().y, child2.getMinXYZ().y);
  minXYZ.z = std::min(child1.getMinXYZ().z, child2.getMinXYZ().z);

  maxXYZ.x = std::max(child1.getMaxXYZ().x, child2.getMaxXYZ().x);
  maxXYZ.y = std::max(child1.getMaxXYZ().y, child2.getMaxXYZ().y);
  maxXYZ.z = std::max(child1.getMaxXYZ().z, child2.getMaxXYZ().z);

  return AABB(minXYZ, maxXYZ);
}

void AABB::printStatus() const {
  std::cout << "AABB:\n"
            << "minXYZ: " << minXYZ.x << '\t' << minXYZ.y << '\t' << minXYZ.z
            << '\n'
            << "maxXYZ: " << maxXYZ.x << '\t' << maxXYZ.y << '\t' << maxXYZ.z
            << '\n';
  std::cout << std::endl;
}

void AABB::hit(const Ray& ray, HitResult& res) const {
  Vec3<float> origin = ray.getOrigin();
  Vec3<float> direction = ray.getDirection();
  Vec3<float> tMin, tMax;

  if (fabs(direction.x) < 0.000001f) {
    // 若射线方向矢量的x轴分量为0且原点不在盒体内
    if (!(minXYZ.x <= origin.x && origin.x <= maxXYZ.x)) {
      res.isHit = false;
      return;
    }
  } else {
    if (direction.x >= 0) {
      tMin.x = (minXYZ.x - origin.x) / direction.x;
      tMax.x = (maxXYZ.x - origin.x) / direction.x;
    } else {
      tMin.x = (maxXYZ.x - origin.x) / direction.x;
      tMax.x = (minXYZ.x - origin.x) / direction.x;
    }
  }

  if (fabs(direction.y) < 0.000001f) {
    // 若射线方向矢量的x轴分量为0且原点不在盒体内
    if (!(minXYZ.y <= origin.y && origin.y <= maxXYZ.y)) {
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

  if (fabs(direction.z) < 0.000001f) {
    // 若射线方向矢量的x轴分量为0且原点不在盒体内
    if (!(minXYZ.z <= origin.z && origin.z <= maxXYZ.z)) {
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
  // 当光线 aabb特别薄时，等号也需要成立
  res.isHit = t0 <= t1;
  return;
}
}  // namespace sre
