#include "../include/Triangle.hpp"

namespace sre {

void Triangle::hit(const Ray& ray, HitResult& res) const {
  Vec3<float> origin = ray.getOrigin();
  Vec3<float> direction = ray.getDirection();
  Vec3<float> normal = this->normal;

  if (Vec3<float>::dot(normal, direction) > 0) {
    // 入射光线与法线必须呈钝角，但后续判断交点是否在三角形内部，需要使用原生法向量
    normal = -normal;
  }

  if (Vec3<float>::dot(normal, direction) == 0) {
    res.isHit = false;
    res.id = this->getId();
    res.normal = normal;
    return;
  }

  float t = (Vec3<float>::dot(normal, v1) - Vec3<float>::dot(normal, origin)) /
            Vec3<float>::dot(normal, direction);
  if (t < 0.001) {
    res.isHit = false;
    res.id = this->getId();
    res.normal = normal;
    res.distance = t;
    return;
  }

  Vec3<float> p = ray.getPointAt(t);
  Vec3<float> c1 = Vec3<float>::cross(v2 - v1, p - v1);
  Vec3<float> c2 = Vec3<float>::cross(v3 - v2, p - v2);
  Vec3<float> c3 = Vec3<float>::cross(v1 - v3, p - v3);
  if (Vec3<float>::dot(c1, this->normal) < 0 ||
      Vec3<float>::dot(c2, this->normal) < 0 ||
      Vec3<float>::dot(c3, this->normal) < 0) {
    res.isHit = false;
    res.id = this->getId();
    res.normal = normal;
    res.distance = t;
    res.hitPoint = p;
    return;
  }

  res.isHit = true;
  res.id = this->getId();
  res.hitPoint = p;
  res.distance = t;
  res.normal = normal;
  res.material = material;
  return;
}

void Triangle::printStatus() const {
  std::cout << "triangle: \n"
            << "id: " << this->getId() << '\n'
            << "vertex 1: " << v1.x << '\t' << v1.y << '\t' << v1.z << '\n'
            << "vertex 2: " << v2.x << '\t' << v2.y << '\t' << v2.z << '\n'
            << "vertex 3: " << v3.x << '\t' << v3.y << '\t' << v3.z << '\n'
            << "normal: " << normal.x << '\t' << normal.y << '\t' << normal.z
            << '\n';
  material.printStatus();
  std::cout << std::endl;
}
}  // namespace sre
