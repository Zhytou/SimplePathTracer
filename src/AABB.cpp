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

bool AABB::intersect(const Ray& ray) const {
    Vec3f org     = ray.getOrigin();
    Vec3f dir     = ray.getDirection();
    Vec3f dir_inv = ray.getInvDirection();
    float tmin    = ray.getTMin();
    float tmax    = ray.getTMax();

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dir[i]) < EPS || std::isinf(dir_inv[i])) {
            if (org[i] < m_xyz1[i] || org[i] > m_xyz2[i]) { return false; }
            continue;
        }
        float t1 = (m_xyz1[i] - org[i]) * dir_inv[i];
        float t2 = (m_xyz2[i] - org[i]) * dir_inv[i];
        tmin     = std::max(tmin, std::min(t1, t2));
        tmax     = std::min(tmax, std::max(t1, t2));
        if (tmin > tmax) { return false; }
    }

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
