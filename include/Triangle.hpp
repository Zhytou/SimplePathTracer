#ifndef SRE_TRIANGLE_HPP
#define SRE_TRIANGLE_HPP

#include <iostream>

#include "Hittable.hpp"

namespace sre {

class Triangle : public Hittable {
 private:
  Vec3<float> p1, p2, p3;  // 三顶点
  Vec3<float> normal;      // 法向量
  Material material;       // 材质

 public:
  Triangle(size_t id, const Vec3<float>& _p1, const Vec3<float>& _p2,
           const Vec3<float>& _p3, const Material& _m)
      : Hittable(id),
        p1(_p1),
        p2(_p2),
        p3(_p3),
        normal(Vec3<float>::normalize(Vec3<float>::cross(p2 - p1, p3 - p1))),
        material(_m) {}
  Triangle(size_t id, const Vec3<float>& _p1, const Vec3<float>& _p2,
           const Vec3<float>& _p3, const Vec3<float>& _n, const Material& _m)
      : Hittable(id),
        p1(_p1),
        p2(_p2),
        p3(_p3),
        normal(Vec3<float>::normalize(_n)),
        material(_m) {}
  ~Triangle() {}

 public:
  // getter
  virtual Vec3<float> getMinXYZ() const override {
    Vec3<float> minXYZ;
    minXYZ.x = std::min(p1.x, std::min(p2.x, p3.x));
    minXYZ.y = std::min(p1.y, std::min(p2.y, p3.y));
    minXYZ.z = std::min(p1.z, std::min(p2.z, p3.z));
    return minXYZ;
  }
  virtual Vec3<float> getMaxXYZ() const override {
    Vec3<float> maxXYZ;
    maxXYZ.x = std::max(p1.x, std::max(p2.x, p3.x));
    maxXYZ.y = std::max(p1.y, std::max(p2.y, p3.y));
    maxXYZ.z = std::max(p1.z, std::max(p2.z, p3.z));
    return maxXYZ;
  }
  Vec3<float> getRandomPoint() const {
    Vec3<float> e1 = p2 - p1, e2 = p3 - p2;
    float a = sqrt(randFloat(1)), b = randFloat(1);
    return e1 * a + e2 * a * b + p1;
  }
  Material getMaterial() const { return material; }
  float getSize() const {
    return Vec3<float>::cross(p2 - p1, p3 - p1).length() / 2;
  }

  // print
  virtual void printStatus() const override;

 public:
  virtual void hit(const Ray& ray, HitResult& res) const override;
};
}  // namespace sre

#endif