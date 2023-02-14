#ifndef SRE_HITTABLE_HPP
#define SRE_HITTABLE_HPP

#include "Material.hpp"
#include "Ray.hpp"
#include "Vec.hpp"

namespace sre {

struct HitResult {
  bool isHit;
  int id;
  double distance;
  Vec3<float> hitPoint;
  Vec3<float> normal;
  Material material;

  HitResult() : isHit(false), distance(-1) {}
};

class Hittable {
 private:
  size_t id;

 public:
  // constructor & destructor
  Hittable(size_t _id = -1) : id(_id) {}
  virtual ~Hittable() {}

 public:
  // getter.
  size_t getId() const { return id; }
  virtual Vec3<float> getMinXYZ() const = 0;
  virtual Vec3<float> getMaxXYZ() const = 0;
  virtual Vec2<float> getTexCoord(const Vec3<float>& coord) const {
    return Vec2<float>(0, 0);
  }

  // print.
  virtual void printStatus() const {}

  virtual void hit(const Ray& ray, HitResult& res) const = 0;
};

}  // namespace sre

#endif