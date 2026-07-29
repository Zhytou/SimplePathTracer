#include "AABB.hpp"
#include "Primitive.hpp"

#include <iostream>

namespace spt {

AABB AABB::create(const std::vector<std::shared_ptr<Primitive>>& primitives) {
    AABB aabb;
    for (const auto& prm : primitives) {
        aabb.merge(prm->wrap());
    }
    return aabb;
}

bool AABB::intersect(const Ray& ray, float tmin, float tmax) const {
    Vec3<float> origin    = ray.getOrigin();
    Vec3<float> direction = ray.getDirection();
    Vec3<float> infdir(1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z);

    float tx1 = (m_xyz1.x - origin.x) * infdir.x;
    float tx2 = (m_xyz2.x - origin.x) * infdir.x;
    tmin      = std::max(tmin, std::min(tx1, tx2));
    tmax      = std::min(tmax, std::max(tx1, tx2));
    if (tmin > tmax) { return false; }

    float ty1 = (m_xyz1.y - origin.y) * infdir.y;
    float ty2 = (m_xyz2.y - origin.y) * infdir.y;
    tmin      = std::max(tmin, std::min(ty1, ty2));
    tmax      = std::min(tmax, std::max(ty1, ty2));
    if (tmin > tmax) { return false; }

    float tz1 = (m_xyz1.z - origin.z) * infdir.z;
    float tz2 = (m_xyz2.z - origin.z) * infdir.z;
    tmin      = std::max(tmin, std::min(tz1, tz2));
    tmax      = std::min(tmax, std::max(tz1, tz2));
    if (tmin > tmax) { return false; }

    return true;
}

AABB& AABB::merge(const AABB& other) {
    m_xyz1.x = std::min(m_xyz1.x, other.m_xyz1.x);
    m_xyz1.y = std::min(m_xyz1.y, other.m_xyz1.y);
    m_xyz1.z = std::min(m_xyz1.z, other.m_xyz1.z);
    m_xyz2.x = std::max(m_xyz2.x, other.m_xyz2.x);
    m_xyz2.y = std::max(m_xyz2.y, other.m_xyz2.y);
    m_xyz2.z = std::max(m_xyz2.z, other.m_xyz2.z);
    return *this;
}

} // namespace spt
