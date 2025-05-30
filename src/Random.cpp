#include "Random.hpp"

namespace spt {

int randInt(int max, int min) { return rand() % (max - min) + min; }

float randFloat(float max, float min) {
  return min + 1.0 * rand() / RAND_MAX * (max - min);
}

Vec3<float> sampleCosine() {
  float a = randFloat(1), b = randFloat(1);
  float cosTheta = sqrtf(a);
  float sinTheta = sqrtf(1 - cosTheta * cosTheta);
  float Phi =  2 * PI * b;
  float x = cosf(Phi) * sinTheta;
  float y = sinf(Phi) * sinTheta;
  float z = cosTheta;

  return Vec3<float>(x, y, z);
}

Vec3<float> sampleGGX(float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;

  float a = randFloat(1), b = randFloat(1);
  float cosTheta = sqrtf((1 - a) / (1 + (alpha2 - 1) * a));
  float sinTheta = sqrtf(1 - cosTheta * cosTheta);
  float Phi = 2 * PI * b;
  float x = cosf(Phi) * sinTheta;
  float y = sinf(Phi) * sinTheta;
  float z = cosTheta;

  return Vec3<float>(x, y, z);
}

}  // namespace spt