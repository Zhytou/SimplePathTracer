#ifndef SRE_LIGHT_HPP
#define SRE_LIGHT_HPP

#include <cassert>
#include <unordered_map>
#include <vector>

#include "Triangle.hpp"
#include "Vec.hpp"

namespace spt {

class Light {
 private:
  std::unordered_map<Vec3<float>, int> lightIds;
  std::vector<std::vector<Triangle>> lightTriangles;
  std::vector<float> lightAreas;

 public:
  Light() = default;
  ~Light() = default;

  // getter
  void getRandomPoint(size_t& id, Vec3<float>& pos, Vec3<float>& radiance,
                      float& area) const;

  // setter
  void setLight(const Triangle& triangle);

  void printStatus() const;
};
}  // namespace spt

#endif