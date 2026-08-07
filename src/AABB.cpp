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

bool AABB::intersect(Ray& ray) const {
    Vec3<float> org = ray.getOrigin();
    Vec3<float> dir = ray.getDirection();
    Vec3<float> dir_inv(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    float tmin = ray.getTMin();
    float tmax = ray.getTMax();

    float tx1 = (m_xyz1.x - org.x) * dir_inv.x;
    float tx2 = (m_xyz2.x - org.x) * dir_inv.x;
    tmin      = std::max(tmin, std::min(tx1, tx2));
    tmax      = std::min(tmax, std::max(tx1, tx2));
    if (tmin > tmax) { return false; }

    float ty1 = (m_xyz1.y - org.y) * dir_inv.y;
    float ty2 = (m_xyz2.y - org.y) * dir_inv.y;
    tmin      = std::max(tmin, std::min(ty1, ty2));
    tmax      = std::min(tmax, std::max(ty1, ty2));
    if (tmin > tmax) { return false; }

    float tz1 = (m_xyz1.z - org.z) * dir_inv.z;
    float tz2 = (m_xyz2.z - org.z) * dir_inv.z;
    tmin      = std::max(tmin, std::min(tz1, tz2));
    tmax      = std::min(tmax, std::max(tz1, tz2));
    if (tmin > tmax) { return false; }

    ray.setTMax(tmax); // update ray t max, ensure that tmax is the minimum of all tmax
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
