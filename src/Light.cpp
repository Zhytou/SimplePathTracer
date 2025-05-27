#include "Light.hpp"
#include "Material.hpp"
#include "Random.hpp"

namespace spt {
void Light::getRandomPoint(size_t& id, Vec3<float>& pos, Vec3<float>& radiance, float& area) const {
  int idx = randInt(triangles.size());

  id = triangles[idx]->getId();
  pos = triangles[idx]->getRandomPoint();
  radiance = triangles[idx]->getMaterial().getEmission();
  area = triangles[idx]->getSize();
}

void Light::setLight(const std::shared_ptr<Triangle>& triangle) {
  triangles.push_back(triangle);
}

void Light::printStatus() const {
  std::cout << "light" << '\n';
  std::cout << "number of light sources: " << triangles.size() << '\n';
  std::cout << std::endl;
}
}  // namespace spt
