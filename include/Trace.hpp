#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "BVH.hpp"
#include "Camera.hpp"
#include "Ray.hpp"
#include "Vec.hpp"

namespace spt {
class Tracer {
 private:
  std::shared_ptr<BVH> scenes;
  std::vector<std::shared_ptr<Hittable>> objects;
  std::vector<std::shared_ptr<Triangle>> lights;
  Camera camera;
  size_t maxDepth;
  size_t samples;
  float thresholdP;

 private:
  bool loadConfiguration(
      const std::string &configName,
      std::unordered_map<std::string, Vec3<float>> &lightRadiances);
  bool loadModel(
      const std::string &modelName, const std::string &pathName,
      const std::unordered_map<std::string, Vec3<float>> &lightRadiances);
  Vec3<float> trace(const Ray &ray, size_t depth);
  void printStatus();

 public:
  Tracer(size_t _depth = 3, size_t _samples = 3, float _p = 0.5);
  ~Tracer() = default;

  void load(const std::string &pathName, const std::vector<std::string> &modelNames,
            const std::string &configName, int bvhMinCount);
  void render(const std::string& imgName = "result.png");
};
}  // namespace spt

#endif