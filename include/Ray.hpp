#ifndef SRE_RAY_HPP
#define SRE_RAY_HPP

#include "Random.hpp"
#include "Vec.hpp"

namespace sre {
class Ray {
 private:
  Vec3<float> origin;
  Vec3<float> direction;

 public:
  Ray() = default;
  ~Ray() = default;
  Ray(const Vec3<float> &org, const Vec3<float> &dir);

  // getter.
  Vec3<float> getOrigin() const;
  Vec3<float> getDirection() const;
  Vec3<float> getPointAt(const float &t) const;
};

// 漫反射光线方向
Vec3<float> diffuseDir(const Vec3<float> &wi, const Vec3<float> &n);
// 镜面反射光线方向
Vec3<float> mirrorDir(const Vec3<float> &wi, const Vec3<float> &n);
// 折射光线方向
Vec3<float> refractDIr(const Vec3<float> &wi, const Vec3<float> &n,
                                    float ior);

}  // namespace sre

#endif