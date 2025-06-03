#ifndef SRE_TRIANGLE_HPP
#define SRE_TRIANGLE_HPP

#include <iostream>

#include "Hittable.hpp"

namespace spt {
class Material;

class Triangle : public Hittable {
 private:
  Vec3<float> v1, v2, v3;
  Vec2<float> vt1, vt2, vt3;
  Vec3<float> normal;
  Material material;

 public:
  Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
           const Vec3<float>& _v3, const Material& _m);
  Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
           const Vec3<float>& _v3, const Vec3<float>& _n, const Material& _m);
  Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
           const Vec3<float>& _v3, const Vec2<float>& _vt1,
           const Vec2<float>& _vt2, const Vec2<float>& _vt3,
           const Vec3<float>& _n, const Material& _m);
  ~Triangle();

 public:
  virtual Vec3<float> getMinXYZ() const override;
  virtual Vec3<float> getMaxXYZ() const override;
  virtual Vec2<float> getTexCoord(const Vec3<float>& coord) const override;
  Vec3<float> getRandomPoint() const;
  Material getMaterial() const;
  float getSize() const;

  // print
  virtual void printStatus() const override;

  // contain
  bool contain(const Vec3<float>& p) const;

  // hit
  virtual void hit(const Ray& ray, HitResult& res) const override;
};
}  // namespace spt

#endif