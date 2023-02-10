#ifndef SRE_HITTABLE_HPP
#define SRE_HITTABLE_HPP

#include "Material.hpp"
#include "Ray.hpp"
#include "Vec.hpp"

struct HitResult {
  int id;
  bool isHit;
  double distance;
  Vec3<float> hitPoint;
  Vec3<float> normal;
  Material material;

  HitResult() : isHit(false), distance(0) {}
  HitResult(const bool& _hit, const double& _dis, const Vec3<float>& _point,
            const Vec3<float>& _n, const Material& _m)
      : isHit(_hit),
        distance(_dis),
        hitPoint(_point),
        normal(_n),
        material(_m) {}
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

  virtual void hit(const Ray& ray, HitResult& res) const = 0;
};

#endif