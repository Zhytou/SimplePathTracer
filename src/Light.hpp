#ifndef SRE_LIGHT_HPP
#define SRE_LIGHT_HPP

#include <cassert>
#include <vector>

#include "Material.hpp"
#include "Random.hpp"
#include "Triangle.hpp"
#include "Vec.hpp"

class Light {
 private:
  std::vector<Triangle> triangles;
  float area;

 public:
  Light() : area(0){};
  ~Light() = default;

  // setter
  void setLight(const Vec3<float>& p1, const Vec3<float>& p2,
                const Vec3<float>& p3, const Material& material) {
    assert(material.isEmissive());
    triangles.emplace_back(p1, p2, p3, material);
    area += cross(p2 - p1, p3 - p1).length();
  }

  // getter
  float getArea() const { return area; }
  std::pair<Vec3<float>, Vec3<float>> getRandomPointAndRadiance() const {
    int idx = randInt(triangles.size());
    return {triangles[idx].getRandomPoint(),
            triangles[idx].getMaterial().getEmission()};
  }
};
#endif