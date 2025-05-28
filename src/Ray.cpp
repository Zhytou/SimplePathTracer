#include "Ray.hpp"

#include <iostream>

namespace spt {
Ray::Ray(const Vec3<float> &org, const Vec3<float> &dir)
    : origin(org), direction(Vec3<float>::normalize(dir)) {}

Vec3<float> Ray::getOrigin() const { return origin; }
Vec3<float> Ray::getDirection() const { return direction; }
Vec3<float> Ray::getPointAt(const float &t) const { return origin + direction * t; }

Vec3<float> diffuseDir(const Vec3<float> &wi, const Vec3<float> &n) {
  Vec3<float> v1 = Vec3<float>::cross(wi, n);
  Vec3<float> v2 = Vec3<float>::cross(v1, n);

  float a = randFloat(1), b = randFloat(1);
  float theta =  2 * PI * a;
  float phi = acos(1 - 2 * b);

  float x = cos(theta) * sin(phi);
  float y = sin(theta) * sin(phi);
  float z = cos(phi);
  
  return v1 * x + v2 * y + n * z;
}

Vec3<float> mirrorDir(const Vec3<float> &wi, const Vec3<float> &n, float roughness) {
  float cosi = Vec3<float>::dot(wi, n);
  Vec3<float> ws = wi - n * 2 * cosi;
  
  Vec3<float> v1 = Vec3<float>::cross(wi, ws).normalize();
  Vec3<float> v2 = Vec3<float>::cross(ws, v1).normalize();
  
  float theta = randFloat(1) * 2 * PI;
  Vec3<float> d = (v1 * cos(theta) + v2 * sin(theta)) * roughness;
  
  return ws + d;
}

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
