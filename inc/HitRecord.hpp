#ifndef SPT_HITRECORD_HPP
#define SPT_HITRECORD_HPP

#include "Utils.hpp"

namespace spt {

class Primitive;

struct HitRecord {
    int id               = -1; // hit primitive id if != -1
    float distance       = INFINITY;
    Vec3<float> point    = Vec3<float>(0.f);
    Vec2<float> texcoord = Vec2<float>(0.f);
    Vec3<float> normal   = Vec3<float>(0.f);
};

} // namespace spt

#endif