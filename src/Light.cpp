#include "../include/Light.hpp"

#include "../include/Material.hpp"
#include "../include/Random.hpp"

namespace sre {
void Light::getRandomPoint(size_t& id, Vec3<float>& pos, Vec3<float>& radiance,
                           float& area) const {
  assert(lightAreas.size() != 0 && lightAreas.size() == lightTriangles.size());
  int idx = randInt(lightAreas.size());
  int triangleIdx = randInt(lightTriangles[idx].size());

  id = lightTriangles[idx][triangleIdx].getId();
  pos = lightTriangles[idx][triangleIdx].getRandomPoint();
  radiance = lightTriangles[idx][triangleIdx].getMaterial().getEmission();
  area = lightAreas[idx];
}

void Light::setLight(const Triangle& triangle) {
  Vec3<float> radiance = triangle.getMaterial().getEmission();
  assert(radiance.x != 0 && radiance.y != 0 && radiance.z != 0);
  assert(lightAreas.size() == lightTriangles.size());
  int id = lightTriangles.size();
  if (lightIds.find(radiance) == lightIds.end()) {
    lightIds[radiance] = id;
    lightTriangles.push_back({});
    lightAreas.push_back(0);
  } else {
    id = lightIds[radiance];
  }
  lightTriangles[id].emplace_back(triangle);
  lightAreas[id] += triangle.getSize();
}
}  // namespace sre
