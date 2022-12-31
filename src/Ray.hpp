#ifndef SRE_RAY_HPP
#define SRE_RAY_HPP

#include "Vec.hpp"

class Ray {
 private:
  Vec3<float> origin;
  Vec3<float> direction;

 public:
  // ctor/dtor.
  Ray() = default;
  ~Ray() = default;
  Ray(const Vec3<float> &org, const Vec3<float> &dir)
      : origin(org), direction(dir) {
    direction = normalize(direction);
  }

  // Getter.
  Vec3<float> getOrigin() const { return origin; }
  Vec3<float> getDirection() const { return direction; }

  // p(t) = origin + t*dir;
  Vec3<float> pointAt(const float &t) const { return origin + direction * t; }
};

#endif