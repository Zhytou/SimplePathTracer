#include "../include/Random.hpp"

namespace sre {

int randInt(int max, int min) { return rand() % (max - min) + min; }

float randFloat(float max, float min) {
  return min + 1.0 * rand() / RAND_MAX * (max - min);
}

}  // namespace sre