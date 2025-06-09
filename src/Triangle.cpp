#include "Triangle.hpp"
#include "Material.hpp"

#include <cassert>

namespace spt {

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Material& _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      normal(normalize(cross(_v2 - _v1, _v3 - _v1))),
      material(_m) {}

Triangle::Triangle(size_t id, const Vec3<float>& _v1, const Vec3<float>& _v2,
                   const Vec3<float>& _v3, const Vec3<float>& _n,
                   const Material& _m)
    : Hittable(id),
      v1(_v1),
      v2(_v2),
      v3(_v3),
      normal(normalize(_n)),
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
      normal(normalize(_n)),
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
  float a = sqrtf(rand(1.f)), b = sqrtf(rand(1.f));
  return e1 * a + e2 * a * b + v1;
}

Vec2<float> Triangle::getTexCoord(const Vec3<float>& coord) const {
  assert(contain(coord));

  Vec3<float> e1 = v2 - v1, e2 = v3 - v1;
  Vec3<float> n = cross(e1, e2);
  float area = n.length();

  // too small to be a valid triangle
  if (area < EPSILON) {
    return vt1;
  }

  // calculate gravity center
  float a = cross(e2, coord - v1).length() / area;
  float b = cross(coord - v1, e1).length() / area;
  float c = 1 - a - b;

  // interpolation
  Vec2<float> texCoord = vt2 * a + vt3 * b + vt1 * c;

  // make sure within the [0, 1] range
  texCoord.u = std::fmod(texCoord.u, 1.0f);
  texCoord.v = std::fmod(texCoord.v, 1.0f);
  if (texCoord.u < 0) texCoord.u += 1.0f;
  if (texCoord.v < 0) texCoord.v += 1.0f;

  return texCoord;
}

Material Triangle::getMaterial() const { return material; }

float Triangle::getSize() const {
  return cross(v2 - v1, v3 - v1).length() / 2;
}

bool Triangle::contain(const Vec3<float>& p) const {
  // edge
  Vec3<float> e1 = v2 - v1;
  Vec3<float> e2 = v3 - v1;

  // vector
  Vec3<float> pv = p - v1;
  
  // pv = u*e1 + v*e2
  float c1 = dot(pv, e1);
  float c2 = dot(pv, e2);
  float c3 = dot(e1, e1);
  float c4 = dot(e2, e2);
  float c5 = dot(e1, e2);

  float denom = c3*c4-c5*c5;
  float u = (c1*c4-c2*c5)/denom;
  float v = (c2*c3-c1*c5)/denom;

  return u >= 0 && v >= 0 && u+v <= 1;
}

void Triangle::hit(const Ray& ray, HitResult& res) const {
  Vec3<float> origin = ray.getOrigin();
  Vec3<float> direction = ray.getDirection();

  // initialize hit result
  res.hit = false;
  res.id = this->getId();

  float denom = dot(normal, direction);
  // ray is parallel to triangle face
  if (fabs(denom) <= EPSILON) {
    return;
  }

  float t = (dot(normal, v1) - dot(normal, origin))/denom;
  Vec3<float> p = ray.getPointAt(t);
  if (t < 0.05 || !contain(p)) {
    return;
  }

  res.hit = true;
  res.point = p;
  res.uv = getTexCoord(p);
  res.distance = t;
  res.normal = normal;
  res.material = material;
  return;
}

}  // namespace spt
