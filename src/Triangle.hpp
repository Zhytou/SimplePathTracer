#ifndef SRE_TRIANGLE_HPP
#define SRE_TRIANGLE_HPP

#include <iostream>

#include "Hittable.hpp"

// 三角形
class Triangle : public Hittable {
 private:
  Vec3<float> p1, p2, p3;  // 三顶点
  Vec3<float> normal;      // 法向量
  Material material;       // 材质

 public:
  Triangle(const Vec3<float>& _p1, const Vec3<float>& _p2,
           const Vec3<float>& _p3, const Material& _m)
      : p1(_p1),
        p2(_p2),
        p3(_p3),
        normal(normalize(cross(p2 - p1, p3 - p1))),
        material(_m) {}
  Triangle(const Vec3<float>& _p1, const Vec3<float>& _p2,
           const Vec3<float>& _p3, const Vec3<float>& _n, const Material& _m)
      : p1(_p1), p2(_p2), p3(_p3), normal(normalize(_n)), material(_m) {}
  ~Triangle() {}

 public:
  // getter
  virtual Vec3<float> getMinXYZ() const override {
    Vec3<float> minXYZ;
    minXYZ.x = std::min(p1.x, std::min(p2.x, p3.x));
    minXYZ.y = std::min(p1.y, std::min(p2.y, p3.y));
    minXYZ.z = std::min(p1.z, std::min(p2.z, p3.z));
    return minXYZ;
  }
  virtual Vec3<float> getMaxXYZ() const override {
    Vec3<float> maxXYZ;
    maxXYZ.x = std::max(p1.x, std::max(p2.x, p3.x));
    maxXYZ.y = std::max(p1.y, std::max(p2.y, p3.y));
    maxXYZ.z = std::max(p1.z, std::max(p2.z, p3.z));
    return maxXYZ;
  }

  Vec3<float> getRandomPoint() const {
    Vec3<float> e1 = p2 - p1, e2 = p3 - p2;
    float a = sqrt(randFloat(1)), b = randFloat(1);
    return e1 * a + e2 * a * b + p1;
  }
  Material getMaterial() const { return material; }

  void printStatus() const {
    std::cout << "vertex 1: " << p1.x << '\t' << p1.y << '\t' << p1.z << '\n'
              << "vertex 2: " << p2.x << '\t' << p2.y << '\t' << p2.z << '\n'
              << "vertex 3: " << p3.x << '\t' << p3.y << '\t' << p3.z << '\n'
              << "normal: " << normal.x << '\t' << normal.y << '\t' << normal.z
              << '\n';
    material.printStatus();
    std::cout << std::endl;
  }

 public:
  // 与光线求交
  virtual void hit(const Ray& ray, HitResult& res) const override {
    Vec3<float> origin = ray.getOrigin();
    Vec3<float> direction = ray.getDirection();
    Vec3<float> normal = this->normal;

    if (dot(normal, direction) > 0) {
      // 入射光线与法线必须呈钝角，但后续判断交点是否在三角形内部，需要使用原生法向量
      normal = -normal;
    }

    if (dot(normal, direction) == 0) {
      res.isHit = false;
      res.normal = normal;
      return;
    }

    float t = (dot(normal, p1) - dot(normal, origin)) / dot(normal, direction);
    if (t < 0.001) {
      res.isHit = false;
      res.normal = normal;
      res.distance = t;
      return;
    }

    Vec3<float> p = ray.pointAt(t);
    Vec3<float> c1 = cross(p2 - p1, p - p1);
    Vec3<float> c2 = cross(p3 - p2, p - p2);
    Vec3<float> c3 = cross(p1 - p3, p - p3);
    if (dot(c1, this->normal) < 0 || dot(c2, this->normal) < 0 ||
        dot(c3, this->normal) < 0) {
      res.isHit = false;
      res.normal = normal;
      res.distance = t;
      res.hitPoint = p;
      return;
    }

    res.isHit = true;
    res.hitPoint = p;
    res.normal = normal;
    res.distance = t;
    res.material = material;
    return;
  };
};

#endif