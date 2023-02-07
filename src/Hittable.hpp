#ifndef SRE_HITTABLE_HPP
#define SRE_HITTABLE_HPP

#include "Material.hpp"
#include "Ray.hpp"
#include "Vec.hpp"

struct HitResult {
  int triangleId;
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
  public:
  virtual void hit(const Ray& ray, HitResult& res) const = 0;
};

#endif