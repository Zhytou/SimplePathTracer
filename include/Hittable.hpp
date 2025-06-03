#ifndef SRE_HITTABLE_HPP
#define SRE_HITTABLE_HPP

#include "Material.hpp"
#include "Ray.hpp"
#include "Utils.hpp"

namespace spt {

struct HitResult {
  bool hit;
  int id;
  float distance;
  Vec3<float> point;
  Vec2<float> uv;
  Vec3<float> normal;
  Material material;

  HitResult() : hit(false), distance(-1) {}
};

class Hittable {
 private:
  size_t id;

 public:
  Hittable(size_t _id = -1) : id(_id) {}
  virtual ~Hittable() {}

 public:
  // getter
  size_t getId() const { return id; }
  virtual Vec3<float> getMinXYZ() const = 0;
  virtual Vec3<float> getMaxXYZ() const = 0;

  // hit
  virtual void hit(const Ray& ray, HitResult& res) const = 0;
};

}  // namespace spt

#endif