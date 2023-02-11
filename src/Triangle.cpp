#include "../include/Triangle.hpp"

namespace sre {

void Triangle::hit(const Ray& ray, HitResult& res) const {
  Vec3<float> origin = ray.getOrigin();
  Vec3<float> direction = ray.getDirection();
  Vec3<float> normal = this->normal;

  if (dot(normal, direction) > 0) {
    // 入射光线与法线必须呈钝角，但后续判断交点是否在三角形内部，需要使用原生法向量
    normal = -normal;
  }

  if (dot(normal, direction) == 0) {
    res.isHit = false;
    res.id = this->getId();
    res.normal = normal;
    return;
  }

  float t = (dot(normal, p1) - dot(normal, origin)) / dot(normal, direction);
  if (t < 0.001) {
    res.isHit = false;
    res.id = this->getId();
    res.normal = normal;
    res.distance = t;
    return;
  }

  Vec3<float> p = ray.getPointAt(t);
  Vec3<float> c1 = cross(p2 - p1, p - p1);
  Vec3<float> c2 = cross(p3 - p2, p - p2);
  Vec3<float> c3 = cross(p1 - p3, p - p3);
  if (dot(c1, this->normal) < 0 || dot(c2, this->normal) < 0 ||
      dot(c3, this->normal) < 0) {
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
            << "vertex 1: " << p1.x << '\t' << p1.y << '\t' << p1.z << '\n'
            << "vertex 2: " << p2.x << '\t' << p2.y << '\t' << p2.z << '\n'
            << "vertex 3: " << p3.x << '\t' << p3.y << '\t' << p3.z << '\n'
            << "normal: " << normal.x << '\t' << normal.y << '\t' << normal.z
            << '\n';
  material.printStatus();
  std::cout << std::endl;
}
}  // namespace sre
