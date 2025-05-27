#include "Ray.hpp"

#include <iostream>

namespace spt {
Ray::Ray(const Vec3<float> &org, const Vec3<float> &dir)
    : origin(org), direction(Vec3<float>::normalize(dir)) {}

Vec3<float> Ray::getOrigin() const { return origin; }
Vec3<float> Ray::getDirection() const { return direction; }
Vec3<float> Ray::getPointAt(const float &t) const { return origin + direction * t; }

// 漫反射光线方向
Vec3<float> diffuseDir(const Vec3<float> &wi, const Vec3<float> &n) {
  // 求出垂直于法向量的任意一对正交基
  Vec3<float> v1 = Vec3<float>::cross(wi, n);
  Vec3<float> v2 = Vec3<float>::cross(v1, n);
  // 生成phi和theta
  float a = randFloat(1), b = randFloat(1);
  float theta =  2 * PI * a;
  float phi = acos(1 - 2 * b);
  float x = cos(theta) * sin(phi);
  float y = sin(theta) * sin(phi);
  float z = cos(phi);
  return v1 * x + v2 * y + n * z;
}

// 镜面反射光线方向
Vec3<float> mirrorDir(const Vec3<float> &wi, const Vec3<float> &n) {
  float cosi = Vec3<float>::dot(wi, n);
  return wi - n * 2 * cosi;
}

// 折射光线方向
Vec3<float> refractDIr(const Vec3<float> &wi, const Vec3<float> &n, float ior) {
  float cosi = Vec3<float>::dot(wi, n);
  double etai = 1, etat = ior;
  return n;

  // if (cosi < 0) {
  //   cosi = -cosi;
  // } else {
  //   std::swap(etai, etat);
  //   n = -n;
  // }

  // float eta = etai / etat;
  // float cos2t = 1 - eta * eta * (1 - cosi * cosi);

  // if (cos2t < 0) {
  //   return ray.direction;
  // } else {
  //   Vec3<float> direction =
  //       ray.direction * eta + n * (eta * cosi - sqrtf(cos2t));
  //   return direction;
  // }
}
}  // namespace spt
