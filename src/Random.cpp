#include "../include/Random.hpp"

namespace sre {

int randInt(int max, int min) { return rand() % (max - min) + min; }

float randFloat(float max, float min) {
  return min + static_cast<float>(rand()) /
                   static_cast<float>(RAND_MAX / (max - min));
}

}  // namespace sre