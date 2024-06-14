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

// 漫反射光线方向
Vec3<float> Ray::randomReflect(const Ray &ray, const Vec3<float> &normal) {
  // 求出垂直于法向量的任意一对正交基
  Vec3<float> v1, v2;
  if (normal.x != 0 || normal.y != 0) {
    v1 = Vec3<float>::normalize(Vec3<float>(normal.y, -normal.x, 0));
  } else {
    v1 = Vec3<float>(1, 0, 0);
  }
  v2 = Vec3<float>::normalize(Vec3<float>::cross(normal, v1));
  // 球面坐标极角phi，方位角theta
  float phi = randFloat(M_PI / 2);
  float theta = randFloat(M_PI * 2);
  return normal * sin(phi) +  v1 * cos(phi) * cos(theta) + v2 * cos(phi) * sin(theta);
}

// 镜面反射光线方向
Vec3<float> Ray::standardReflect(const Ray &ray, const Vec3<float> &normal) {
  float cosi = Vec3<float>::dot(normal, ray.direction);
  return ray.direction - normal * 2 * cosi;
}

// 折射光线方向
Vec3<float> Ray::standardRefract(const Ray &ray, const Vec3<float> &normal,
                                 float ior) {
  float cosi = Vec3<float>::dot(ray.direction, normal);
  double etai = 1, etat = ior;
  Vec3<float> n = normal;

  if (cosi < 0) {
    cosi = -cosi;
  } else {
    std::swap(etai, etat);
    n = -n;
  }

  float eta = etai / etat;
  float cos2t = 1 - eta * eta * (1 - cosi * cosi);

  if (cos2t < 0) {
    return ray.direction;
  } else {
    Vec3<float> direction =
        ray.direction * eta + n * (eta * cosi - sqrtf(cos2t));
    return direction;
  }
}
}  // namespace sre
