#ifndef SRE_LIGHT_HPP
#define SRE_LIGHT_HPP

#include <cassert>
#include <vector>

#include "Material.hpp"
#include "Random.hpp"
#include "Vec.hpp"

class Light {
 private:
  std::unordered_map<Vec3<float>, int> lightIds;
  std::vector<std::vector<Triangle>> lightTriangles;
  std::vector<float> lightAreas;

 public:
  Light() = default;
  ~Light() = default;

  // setter
  void setLight(const Triangle& triangle) {
    Vec3<float> radiance = triangle.getMaterial().getEmission();
    assert(radiance.x != 0 && radiance.y != 0 && radiance.z != 0);
    assert(lightAreas.size() == lightTriangles.size());
    int id = lightTriangles.size();
    if (lightIds.find(radiance) == lightIds.end()) {
      lightIds[radiance] = id;
      lightAreas.push_back(0);
    } else {
      id = lightIds[radiance];
    }
    lightTriangles[id].emplace_back(triangle);
    lightAreas[id] += triangle.getSize();
  }

  // getter
  void getRandomPoint(size_t& id, Vec3<float>& pos, Vec3<float>& radiance, float& area) const {
    assert(lightAreas.size() == lightTriangles.size());
    int idx = randInt(lightAreas.size());
    int triangleIdx = randInt(lightTriangles[idx].size());

    id = lightTriangles[idx][triangleIdx].getId();
    pos = lightTriangles[idx][triangleIdx].getRandomPoint();
    radiance = lightTriangles[idx][triangleIdx].getMaterial().getEmission();
    area = lightAreas[idx];
  }
};

#endif