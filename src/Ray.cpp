#include "../include/Ray.hpp"

#include <iostream>

namespace sre {
Ray::Ray(const Vec3<float> &org, const Vec3<float> &dir)
    : origin(org), direction(Vec3<float>::normalize(dir)) {}

// getter.
Vec3<float> Ray::getOrigin() const { return origin; }
Vec3<float> Ray::getDirection() const { return direction; }
Vec3<float> Ray::getPointAt(const float &t) const {
  return origin + direction * t;
}

// 折射光线
Ray Ray::standardRefractRay(const Vec3<float> &point,
                            const Vec3<float> &direction,
                            const Vec3<float> &normal, float refraction) {
  float cosineIncidence = fabs(Vec3<float>::dot(direction, normal));
  float sineIncidence = sqrt(1 - cosineIncidence * cosineIncidence);
  float sineRefraction = sineIncidence / refraction;
  float cosineRefraction = sqrt(1 - sineRefraction * sineRefraction);

  // std::cout << cosineIncidence << '\t' << sineIncidence << '\t'
  //           << sineRefraction << '\t' << cosineRefraction << '\n';

  Vec3<float> directionParallelN = normal * Vec3<float>::dot(normal, direction);
  Vec3<float> directionVerticalN = direction - directionParallelN;

  // std::cout << "Paralle d:" << directionParallelN.x << '\t'
  //           << directionParallelN.y << '\t' << directionParallelN.z << '\n';
  // std::cout << "Vertical d:" << directionVerticalN.x << '\t'
  //           << directionVerticalN.y << '\t' << directionVerticalN.z << '\n';

  Vec3<float> ndirectionParallelN(0, 0, 0);
  if (cosineIncidence != 0) {
    ndirectionParallelN =
        directionParallelN / cosineIncidence * cosineRefraction;
  }

  Vec3<float> ndirectionVerticalN(0, 0, 0);
  if (sineIncidence != 0) {
    ndirectionVerticalN = directionVerticalN / sineIncidence * sineRefraction;
  }

  return Ray(point, ndirectionParallelN + ndirectionVerticalN);
}

// 镜面反射光线
Ray Ray::standardReflectRay(const Vec3<float> &point,
                            const Vec3<float> &direction,
                            const Vec3<float> &normal) {
  // 分解入射光
  Vec3<float> directionParallelN = normal * Vec3<float>::dot(normal, direction);
  Vec3<float> directionVerticalN = direction - directionParallelN;
  // 反射光
  Vec3<float> ndirection = -directionParallelN + directionVerticalN;
  return Ray(point, ndirection);
}

// 漫反射光线
Ray Ray::randomReflectRay(const Vec3<float> &point,
                          const Vec3<float> &direction,
                          const Vec3<float> &normal) {
  Vec3<float> ndirection(0, 0, 0);
  do {
    ndirection = Vec3<float>::normalize(normal) +
                 Vec3<float>::normalize(Vec3<float>(
                     randFloat(1, -1), randFloat(1, -1), randFloat(1, -0.95)));
    ndirection.normalize();
  } while (Vec3<float>::dot(direction, ndirection) == -1);
  return Ray(point, ndirection);
}
}  // namespace sre
