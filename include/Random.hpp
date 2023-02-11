#ifndef SRE_RANDOM_HPP
#define SRE_RANDOM_HPP

#include <cstdlib>

namespace sre {

int randInt(int max, int min = 0) { return rand() % (max - min) + min; }

float randFloat(float max, float min = 0) {
  return min + static_cast<float>(rand()) /
                   static_cast<float>(RAND_MAX / (max - min));
}

}  // namespace sre

#endif