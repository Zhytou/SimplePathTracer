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

  // 漫反射光线
  static Ray randomReflectRay(const Vec3<float> &point,
                              const Vec3<float> &direction,
                              const Vec3<float> &normal);
  // 镜面反射光线
  static Ray standardReflectRay(const Vec3<float> &point,
                                const Vec3<float> &direction,
                                const Vec3<float> &normal);
  // 折射光线
  static Ray standardRefractRay(const Vec3<float> &point,
                                const Vec3<float> &direction,
                                const Vec3<float> &normal, float refraction);
};

}  // namespace sre

#endif