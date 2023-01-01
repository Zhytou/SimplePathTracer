#ifndef SRE_RAY_HPP
#define SRE_RAY_HPP

#include "Random.hpp"
#include "Vec.hpp"

class Ray {
 private:
  Vec3<float> origin;
  Vec3<float> direction;

 public:
  Ray() = default;
  ~Ray() = default;
  Ray(const Vec3<float> &org, const Vec3<float> &dir)
      : origin(org), direction(dir) {
    direction.normalize();
  }

  // Getter.
  Vec3<float> getOrigin() const { return origin; }
  Vec3<float> getDirection() const { return direction; }

  // p(t) = origin + t*direction;
  Vec3<float> pointAt(const float &t) const { return origin + direction * t; }
};

Ray randomRay(const Vec3<float> &point, const Vec3<float> &normal) {
  Vec3<float> direction = normal;
  direction.normalize();
  direction += Vec3<float>(randFloat(1), randFloat(1), randFloat(1));

  return Ray(point, direction);
}

#endif