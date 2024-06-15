#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "BVH.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Ray.hpp"
#include "Vec.hpp"

namespace sre {
class Tracer {
 private:
  BVH *scenes;
  std::vector<Hittable *> objects;
  Camera camera;
  Light light;
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
  ~Tracer();

  void load(const std::string &pathName, const std::vector<std::string> &modelNames,
            const std::string &configName);
  cv::Mat render();
};
}  // namespace sre

#endif