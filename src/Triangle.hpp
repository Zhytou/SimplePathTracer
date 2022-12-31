#ifndef SRE_TRIANGLE_HPP
#define SRE_TRIANGLE_HPP

#include "Material.hpp"
#include "Ray.hpp"
#include "Vec.hpp"

struct HitResult {
  bool isHit;
  double distance;
  Vec3<float> hitPoint;
  Material material;

  HitResult() : isHit(false), distance(0) {}
  HitResult(const bool& _hit, double _dis, Vec3<float> _point,
            const Material& _m)
      : isHit(_hit), distance(_dis), hitPoint(_point), material(_m) {}
};

// 三角形
class Triangle {
 private:
  Vec3<float> p1, p2, p3;     // 三顶点
  Vec3<float> normal;         // 法向量
  Vec3<unsigned char> color;  //
  Material material;          // 材质

 public:
  Triangle(const Vec3<float>& _p1, const Vec3<float>& _p2,
           const Vec3<float>& _p3, const Material& _m)
      : p1(_p1),
        p2(_p2),
        p3(_p3),
        normal(cross(p2 - p1, p3 - p1)),
        material(_m) {}
  Triangle(const Vec3<float>& _p1, const Vec3<float>& _p2,
           const Vec3<float>& _p3, const Vec3<float>& _n, const Material& _m)
      : p1(_p1), p2(_p2), p3(_p3), normal(_n), material(_m) {}

  // 与光线求交
  HitResult hit(const Ray& ray) const {
    HitResult res;

    Vec3<float> origin = ray.getOrigin();
    Vec3<float> direction = ray.getDirection();

    if (dot(normal, direction) == 0) {
      return res;
    }

    float t = (dot(normal, p1) - dot(normal, origin) / dot(normal, direction));
    if (t < 0) {
      return res;
    }

    Vec3<float> p = ray.pointAt(t);
    Vec3<float> c1 = cross(p2 - p1, p - p1);
    Vec3<float> c2 = cross(p3 - p2, p - p2);
    Vec3<float> c3 = cross(p1 - p3, p - p3);
    if (dot(c1, normal) < 0 || dot(c2, normal) < 0 || dot(c3, normal) < 0) {
      return res;
    }

    res.isHit = true;
    res.hitPoint = p;
    res.distance = t;
    res.material = material;
    return res;
  };
};

#endif