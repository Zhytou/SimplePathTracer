#ifndef SRE_TRIANGLE_HPP
#define SRE_TRIANGLE_HPP

#include <iostream>

#include "Hittable.hpp"

namespace sre {

class Triangle : public Hittable {
 private:
  Vec3<float> v1, v2, v3;  // 顶点坐标
  Vec2<float> vt1, vt2, vt3;   // 纹理坐标
  Vec3<float> normal;      // 法向量
  Material material;       // 材质

 public:
  Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
           const Vec3<float>& _v3, const Material& _m)
      : Hittable(id),
        v1(_v1),
        v2(_v2),
        v3(_v3),
        normal(Vec3<float>::normalize(Vec3<float>::cross(_v2 - _v1, _v3 - _v1))),
        material(_m) {}
  Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
           const Vec3<float>& _v3, const Vec3<float>& _n, const Material& _m)
      : Hittable(id),
        v1(_v1),
        v2(_v2),
        v3(_v3),
        normal(Vec3<float>::normalize(_n)),
        material(_m) {}
  Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
           const Vec3<float>& _v3, const Vec2<float>& _vt1, const Vec2<float>& _vt2, 
           const Vec2<float>& _vt3, const Vec3<float>& _n, const Material& _m)
      : Hittable(id),
        v1(_v1),
        v2(_v2),
        v3(_v3),
        vt1(_vt1),
        vt2(_vt2),
        vt3(_vt3),
        normal(Vec3<float>::normalize(_n)),
        material(_m) {}
  ~Triangle() {}

 public:
  // getter
  virtual Vec3<float> getMinXYZ() const override {
    Vec3<float> minXYZ;
    minXYZ.x = std::min(v1.x, std::min(v2.x, v3.x));
    minXYZ.y = std::min(v1.y, std::min(v2.y, v3.y));
    minXYZ.z = std::min(v1.z, std::min(v2.z, v3.z));
    return minXYZ;
  }
  virtual Vec3<float> getMaxXYZ() const override {
    Vec3<float> maxXYZ;
    maxXYZ.x = std::max(v1.x, std::max(v2.x, v3.x));
    maxXYZ.y = std::max(v1.y, std::max(v2.y, v3.y));
    maxXYZ.z = std::max(v1.z, std::max(v2.z, v3.z));
    return maxXYZ;
  }
  Vec3<float> getRandomPoint() const {
    Vec3<float> e1 = v2 - v1, e2 = v3 - v2;
    float a = sqrt(randFloat(1)), b = randFloat(1);
    return e1 * a + e2 * a * b + v1;
  }
  Material getMaterial() const { return material; }
  float getSize() const {
    return Vec3<float>::cross(v2 - v1, v3 - v1).length() / 2;
  }

  // setter
  void setTextureVertex(const Vec2<float>& _vt1, const Vec2<float>& _vt2, const Vec2<float>& _vt3) {
    vt1 = _vt1;
    vt2 = _vt2;
    vt3 = _vt3;
  }

  // print
  virtual void printStatus() const override;

 public:
  virtual void hit(const Ray& ray, HitResult& res) const override;
};
}  // namespace sre

#endif