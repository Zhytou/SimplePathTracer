#ifndef SPT_INTERSECTION_HPP
#define SPT_INTERSECTION_HPP

#include "Utils.hpp"

namespace spt {

struct Intersection {
    int id                = -1; // hit primitive id if != -1
    float distance        = INFINITY;
    Vec3<float> point     = Vec3<float>(0.f);
    Vec2<float> texcoord  = Vec2<float>(0.f);
    Vec3<float> normal    = Vec3<float>(0.f);
    Vec3<float> tangent   = Vec3<float>(0.f);
    Vec3<float> bitangent = Vec3<float>(0.f);

    /**
     * @brief Transform a direction vector from local to world space
     * 
     * @param dir Direction vector in local space
     * @return Direction vector in world space
     */
    Vec3<float> toWorld(const Vec3<float>& dir) const {
        return spt::toWorld(dir, tangent, bitangent, normal);
    }

    /**
     * @brief Transform a direction vector from world to local space
     * 
     * @param dir Direction vector in world space
     * @return Direction vector in local space
     */
    Vec3<float> toLocal(const Vec3<float>& dir) const {
        return spt::toLocal(dir, tangent, bitangent, normal);
    }
};

} // namespace spt

#endif