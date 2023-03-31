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

// 漫反射光线
Ray Ray::randomReflectRay(const Ray &ray, const Vec3<float> &normal) {
  Vec3<float> direction(0, 0, 0);
  do {
    direction = Vec3<float>::normalize(normal) +
                Vec3<float>::normalize(Vec3<float>(
                    randFloat(1, -1), randFloat(1, -1), randFloat(1, -0.95)));
    direction.normalize();
  } while (Vec3<float>::dot(ray.direction, direction) == -1);
  return Ray(ray.origin, direction);
}

// 镜面反射光线
Ray Ray::standardReflectRay(const Ray &ray, const Vec3<float> &normal) {
  float cosine = Vec3<float>::dot(normal, ray.direction);
  return Ray(ray.origin, ray.direction - normal * 2 * cosine);
}

// 折射光线
Ray Ray::standardRefractRay(const Ray &ray, const Vec3<float> &normal,
                            float ior) {
  float cosine = Vec3<float>::dot(ray.direction, normal);
  double etai = 1, etat = ior;
  Vec3<float> n = normal;

  if (cosine < 0) {
    cosine = -cosine;
  } else {
    std::swap(etai, etat);
    n = -n;
  }

  double eta = etai / etat;
  double k = 1 - eta * eta * (1 - cosine * cosine);

  if (k < 0) {
    return ray;
  } else {
    Vec3<float> direction = ray.direction * eta + n * (eta * cosine - sqrt(k));
    Vec3<float> origin = ray.origin + direction * 0.5;
    return Ray(origin, direction);
  }
}
}  // namespace sre
