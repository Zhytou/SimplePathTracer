#include "Triangle.hpp"

#include <cassert>

namespace spt {

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Material& _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      normal(Vec3<float>::normalize(Vec3<float>::cross(_v2 - _v1, _v3 - _v1))),
      material(_m) {}

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Vec3<float>& _n,
                   const Material& _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      normal(Vec3<float>::normalize(_n)),
      material(_m) {}

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Vec2<float>& _vt1,
                   const Vec2<float>& _vt2, const Vec2<float>& _vt3,
                   const Vec3<float>& _n, const Material& _m)
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

Vec2<float> Triangle::getTexCoord(const Vec3<float>& coord) const {
  Vec3<float> e1 = v2 - v1, e2 = v3 - v1;
  Vec3<float> e = coord - v1;
  float a, b;
  if (e1.x * e2.y != e1.y * e2.x) {
    a = (e.y * e2.x - e.x * e2.y) / (e2.x * e1.y - e1.x * e2.y);
    b = (e.x * e1.y - e.y * e1.x) / (e2.x * e1.y - e1.x * e2.y);
  } else if (e1.x * e2.z != e1.z * e2.x) {
    a = (e.x * e2.z - e.z * e2.x) / (e1.x * e2.z - e1.z * e2.x);
    b = (e.z * e1.x - e.x * e1.z) / (e1.x * e2.z - e1.z * e2.x);
  } else if (e1.y * e2.z != e1.z * e2.y) {
    a = (e.y * e2.z - e.z * e2.y) / (e1.y * e2.z - e1.z * e2.y);
    b = (e.z * e1.z - e.y * e1.z) / (e1.y * e2.z - e1.z * e2.y);
  } else {
    a = 0;
    b = 0;
  }
  Vec2<float> texCoord(0, 0);
  texCoord = vt1 + (vt2 - vt1) * a + (vt3 - vt1) * b;
  // if (texCoord.u >= 0 && texCoord.u <= 1 && texCoord.v >= 0 &&
  //     texCoord.v <= 1) {
  //   std::cout << "tex coord fail!\n"
  //             << "a = " << a << " b = " << b << '\n';
  //   this->printStatus();
  // }

  // 保证纹理坐标都在[0, 1]的范围内
  while (texCoord.u < 0 || texCoord.u > 1) {
    texCoord.u += texCoord.u < 0 ? 1 : -1;
  }
  while (texCoord.v < 0 || texCoord.v > 1) {
    texCoord.v += texCoord.v < 0 ? 1 : -1;
  }
  return texCoord;
}

Material Triangle::getMaterial() const { return material; }

float Triangle::getSize() const {
  return Vec3<float>::cross(v2 - v1, v3 - v1).length() / 2;
}

bool Triangle::contain(const Vec3<float>& p) const {
  // edge
  Vec3<float> e1 = v2 - v1;
  Vec3<float> e2 = v3 - v1;

  // vector
  Vec3<float> pv = p - v1;
  
  // pv = u*e1 + v*e2
  float c1 = Vec3<float>::dot(pv, e1);
  float c2 = Vec3<float>::dot(pv, e2);
  float c3 = Vec3<float>::dot(e1, e1);
  float c4 = Vec3<float>::dot(e2, e2);
  float c5 = Vec3<float>::dot(e1, e2);

  float denom = c3*c4-c5*c5;
  float u = (c1*c4-c2*c5)/denom;
  float v = (c2*c3-c1*c5)/denom;

  return u >= 0 && v >= 0 && u+v <= 1;
}

void Triangle::hit(const Ray& ray, HitResult& res) const {
  Vec3<float> origin = ray.getOrigin();
  Vec3<float> direction = ray.getDirection();

  // initialize hit result
  res.isHit = false;
  res.id = this->getId();

  float denom = Vec3<float>::dot(normal, direction);
  if (fabs(denom) <= 1e-6) {
    return;
  }

  float t = (Vec3<float>::dot(normal, v1)-Vec3<float>::dot(normal, origin))/denom;
  Vec3<float> p = ray.getPointAt(t);
  if (t < 0.05 || !contain(p)) {
    return;
  }

  res.isHit = true;
  res.hitPoint = p;
  res.distance = t;
  res.normal = normal;
  res.material = material;
  return;
}

void Triangle::printStatus() const {
  std::cout << "triangle: \n"
            << "id: " << this->getId() << '\n'
            << "material: " << material.getName() << '\n'
            << "vertex 1: " << v1.x << '\t' << v1.y << '\t' << v1.z << '\n'
            << "vertex 2: " << v2.x << '\t' << v2.y << '\t' << v2.z << '\n'
            << "vertex 3: " << v3.x << '\t' << v3.y << '\t' << v3.z << '\n'
            << "vertex 1 texture: " << vt1.u << '\t' << vt1.v << '\n'
            << "vertex 2 texture: " << vt2.u << '\t' << vt2.v << '\n'
            << "vertex 3 texture: " << vt3.u << '\t' << vt3.v << '\n'
            << "normal: " << normal.x << '\t' << normal.y << '\t' << normal.z
            << '\n';
  std::cout << std::endl;
}
}  // namespace spt
