#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "BVH.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "Ray.hpp"

namespace spt {
class Tracer {
 private:
  std::shared_ptr<BVH> scene;
  Light light;
  Camera camera;
  size_t maxDepth;
  size_t samples;
  float maxProb;

 private:
  bool loadConfig(const std::string &config, std::unordered_map<std::string, Vec3<float>> &lightRadiances, uint& illuType);
  bool loadModel(const std::string &model, const std::string &dir, const std::unordered_map<std::string, Vec3<float>> &lightRadiances, uint illuType, std::vector<std::shared_ptr<Hittable>>& objects);
  Vec3<float> trace(const Ray &ray, size_t depth);

  void print() const;
  static void showProgress(float percent);

 public:
  Tracer(size_t _depth = 3, size_t _samples = 3, float _p = 0.5);
  ~Tracer() = default;

  void load(const std::string &dir, const std::vector<std::string> &models, const std::string &config, int bvhMinCount);
  void render(const std::string& imgName = "result.png");
};
}  // namespace spt

#endif