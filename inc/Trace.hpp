#ifndef SRE_TRACE_HPP
#define SRE_TRACE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "BVH.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "Ray.hpp"

namespace spt {
class Tracer {
private:
  std::shared_ptr<BVH> scene;
  Light light;
  Camera camera;
  size_t maxd; // max depth of path trace
  size_t rrd;  // depth of russian roulette
  size_t spp;  // samples per pixel
  float rrp;   // probability of russian roulette

private:
  Vec3<float> trace(const Ray &ray, size_t depth);

  void print() const;
  static void showProgress(float percent, float second);

public:
  Tracer(size_t depth = 10, size_t rrdepth = 3, size_t samples = 3, float p = 0.8);
  ~Tracer() = default;

  bool load(const std::string &dir, const std::string &config, int bvhMinCount = 20);
  void render(const std::string &imgName = "result.png");
};

float mix(float, float);
} // namespace spt

#endif