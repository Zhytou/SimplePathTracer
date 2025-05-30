#ifndef SRE_RANDOM_HPP
#define SRE_RANDOM_HPP

#include <cstdlib>
#include "Vec.hpp"

namespace spt {

int randInt(int max, int min = 0);

float randFloat(float max, float min = 0);

Vec3<float> sampleCosine();

Vec3<float> sampleGGX(float roughness);

}  // namespace spt

#endif