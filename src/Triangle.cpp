#include <cassert>

#include "../include/Material.hpp"
#include "../include/Triangle.hpp"

namespace sre {

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, Material* _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      normal(Vec3<float>::normalize(Vec3<float>::cross(_v2 - _v1, _v3 - _v1))),
      material(_m) {}

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Vec3<float>& _n,
                   Material* _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      normal(Vec3<float>::normalize(_n)),
      material(_m) {}

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Vec2<float>& _vt1,
                   const Vec2<float>& _vt2, const Vec2<float>& _vt3,
                   const Vec3<float>& _n, Material* _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      vt1(_vt1),
      vt2(_vt2),
      vt3(_vt3),
      normal(Vec3<float>::normalize(_n)),
      material(_m) {}

Triangle::~Triangle() {}

Vec3<float> Triangle::getMinXYZ() const {
  Vec3<float> minXYZ;
  minXYZ.x = std::min(v1.x, std::min(v2.x, v3.x));
  minXYZ.y = std::min(v1.y, std::min(v2.y, v3.y));
  minXYZ.z = std::min(v1.z, std::min(v2.z, v3.z));
  return minXYZ;
}

Vec3<float> Triangle::getMaxXYZ() const {
  Vec3<float> maxXYZ;
  maxXYZ.x = std::max(v1.x, std::max(v2.x, v3.x));
  maxXYZ.y = std::max(v1.y, std::max(v2.y, v3.y));
  maxXYZ.z = std::max(v1.z, std::max(v2.z, v3.z));
  return maxXYZ;
}

Vec3<float> Triangle::getRandomPoint() const {
  Vec3<float> e1 = v2 - v1, e2 = v3 - v2;
  float a = sqrt(randFloat(1)), b = randFloat(1);
  return e1 * a + e2 * a * b + v1;
}

Vec2<float> Triangle::getTexCoord(const Vec3<float>& point) const {
  Vec3<float> e1 = v2 - v1, e2 = v3 - v1;
  Vec3<float> e = point - v1;
  float a, b;
  if (e1.x * e2.y != e1.y * e2.x) {
    a = (e.y * e2.x - e.x * e2.y) / (e2.x * e1.y - e1.x * e2.y);
    b = (e.x * e1.y - e.y * e1.x) / (e2.x * e1.y - e1.x * e2.y);
  } else if (e1.x * e2.z != e1.z * e2.x) {
    a = (e.x * e2.z - e.z * e2.x) / (e1.x * e2.z - e1.z * e2.x);
    b = (e.z * e1.x - e.x * e1.z) / (e1.x * e2.z - e1.z * e2.x);
  } else {
    a = (e.y * e2.z - e.z * e2.y) / (e1.y * e2.z - e1.z * e2.y);
    b = (e.z * e1.z - e.y * e1.z) / (e1.y * e2.z - e1.z * e2.y);
  }
  Vec2<float> texCoord;
  texCoord = vt1 + (vt2 - vt1) * a + (vt3 - vt1) * b;
  return texCoord;
}

Material Triangle::getMaterial() const { 
  assert(material != nullptr);
  return *material;
}

float Triangle::getSize() const {
  return Vec3<float>::cross(v2 - v1, v3 - v1).length() / 2;
}

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
  std::cout << std::endl;
}
}  // namespace sre
