#ifndef SRE_LIGHT_HPP
#define SRE_LIGHT_HPP

#include <cassert>
#include <vector>
#include <memory>

#include "Triangle.hpp"
#include "Vec.hpp"

namespace spt {

class Light {
 private:
  std::vector<std::shared_ptr<Triangle>> triangles;

 public:
  Light() = default;
  ~Light() = default;

  // getter
  void getRandomPoint(size_t& id, Vec3<float>& pos, Vec3<float>& radiance, float& area) const;

  // setter
  void setLight(const std::shared_ptr<Triangle>& triangle);

  void printStatus() const;
};
}  // namespace spt

#endif