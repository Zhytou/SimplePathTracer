#ifndef SPT_INTERSECTION_HPP
#define SPT_INTERSECTION_HPP

#include "Utils.hpp"

namespace spt {

struct Intersection {
    int id          = -1; // hit primitive id if != -1
    float distance  = INF;
    Vec3f point     = Vec3f(0.f);
    Vec2f texcoord  = Vec2f(0.f);
    Vec3f normal    = Vec3f(0.f);
    Vec3f tangent   = Vec3f(0.f);
    Vec3f bitangent = Vec3f(0.f);

    /**
     * @brief Transform a direction vector from local to world space
     * 
     * @param dir Direction vector in local space
     * @return Direction vector in world space
     */
    Vec3f toWorld(const Vec3f& dir) const {
        return spt::toWorld(dir, tangent, bitangent, normal);
    }

    /**
     * @brief Transform a direction vector from world to local space
     * 
     * @param dir Direction vector in world space
     * @return Direction vector in local space
     */
    Vec3f toLocal(const Vec3f& dir) const {
        return spt::toLocal(dir, tangent, bitangent, normal);
    }
};

} // namespace spt

#endif