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
      : origin(org), direction(normalize(dir)) {}

  // Getter.
  Vec3<float> getOrigin() const { return origin; }
  Vec3<float> getDirection() const { return direction; }

  // p(t) = origin + t*direction;
  Vec3<float> pointAt(const float &t) const { return origin + direction * t; }
};

// 折射光线
Ray standardRefractRay(const Vec3<float> &point, const Vec3<float> &direction,
                       const Vec3<float> &normal, float refraction) {
  float cosineIncidence = fabs(dot(direction, normal));
  float sineIncidence = sqrt(1 - cosineIncidence * cosineIncidence);
  float sineRefraction = sineIncidence / refraction;
  float cosineRefraction = sqrt(1 - sineRefraction * sineRefraction);
  Vec3<float> directionParallelN = normal * dot(normal, direction);
  Vec3<float> directionVerticalN = direction - directionParallelN;
  Vec3<float> ndirectionParallelN =
      directionParallelN / cosineIncidence * cosineRefraction;
  Vec3<float> ndirectionVerticalN =
      directionVerticalN / sineIncidence * sineRefraction;
  return Ray(point, ndirectionParallelN + ndirectionVerticalN);
}

// 镜面反射光线
Ray standardReflectRay(const Vec3<float> &point, const Vec3<float> &direction,
                       const Vec3<float> &normal) {
  // 分解入射光
  Vec3<float> directionParallelN = normal * dot(normal, direction);
  Vec3<float> directionVerticalN = direction - directionParallelN;
  // 反射光
  Vec3<float> ndirection = -directionParallelN + directionVerticalN;
  return Ray(point, ndirection);
}

// 漫反射光线
Ray randomReflectRay(const Vec3<float> &point, const Vec3<float> &normal) {
  Vec3<float> direction =
      normalize(normal) +
      normalize(
          Vec3<float>(randFloat(1, -1), randFloat(1, -1), randFloat(1, -0.95)));
  return Ray(point, direction);
}

#endif