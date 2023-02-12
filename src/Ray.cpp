#include "../include/Ray.hpp"

namespace sre {
// 折射光线
Ray Ray::standardRefractRay(const Vec3<float> &point,
                            const Vec3<float> &direction,
                            const Vec3<float> &normal, float refraction) {
  float cosineIncidence = fabs(Vec3<float>::dot(direction, normal));
  float sineIncidence = sqrt(1 - cosineIncidence * cosineIncidence);
  float sineRefraction = sineIncidence / refraction;
  float cosineRefraction = sqrt(1 - sineRefraction * sineRefraction);
  Vec3<float> directionParallelN = normal * Vec3<float>::dot(normal, direction);
  Vec3<float> directionVerticalN = direction - directionParallelN;
  Vec3<float> ndirectionParallelN =
      directionParallelN / cosineIncidence * cosineRefraction;
  Vec3<float> ndirectionVerticalN =
      directionVerticalN / sineIncidence * sineRefraction;
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
