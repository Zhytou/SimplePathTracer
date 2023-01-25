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
  std::vector<std::vector<int>> triangleIds;
  std::vector<float> lightAreas;

 public:
  Light() = default;
  ~Light() = default;

  // setter
  void setLight(const int& triangleId, const float& area,
                const Vec3<float>& radiance) {
    assert(radiance.x != 0 && radiance.y != 0 && radiance.z != 0);
    assert(lightAreas.size() == triangleIds.size());
    int id = triangleIds.size();
    if (lightIds.find(radiance) == lightIds.end()) {
      triangleIds.emplace_back(0);
      lightAreas.push_back(0);
    } else {
      id = lightIds[radiance];
    }
    triangleIds[id].push_back(triangleId);
    lightAreas[id] += area;
  }

  // getter
  // 获取光源数量
  int getLightSize() const {
    assert(lightAreas.size() == triangleIds.size());
    return lightAreas.size();
  }
  // 获取光源面积
  float getLightAreaAt(const int& idx) const { return lightAreas[idx]; }
  int getRandomTriangleId(const int& lightId) const {
    int triangleId = randInt(triangleIds[lightId].size());
    return triangleId;
  }
};
#endif