#ifndef SRE_RAY_HPP
#define SRE_RAY_HPP

#include "Random.hpp"
#include "Vec.hpp"

namespace spt {
class Ray {
 private:
  Vec3<float> origin;
  Vec3<float> direction;

 public:
  Ray() = default;
  ~Ray() = default;
  Ray(const Vec3<float> &org, const Vec3<float> &dir) : origin(org), direction(Vec3<float>::normalize(dir)) {}

  // getter
  Vec3<float> getOrigin() const { return origin; }
  Vec3<float> getDirection() const { return direction; }
  Vec3<float> getPointAt(const float &t) const { return origin + direction * t; }
};

}  // namespace spt

#endif