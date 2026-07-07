#ifndef SPT_AABB_HPP
#define SPT_AABB_HPP

#include <cassert>
#include <cfloat>
#include <memory>
#include <vector>

#include "Ray.hpp"
#include "Utils.hpp"

namespace spt {

class AABB {
   public:
    AABB()  = default;
    ~AABB() = default;
    AABB(const Vec3<float>& xyz1, const Vec3<float>& xyz2) : m_xyz1(xyz1), m_xyz2(xyz2) {}

    bool intersect(const Ray& ray, float tmin, float tmax) const;
    AABB& merge(const AABB& other);

    Vec3<float> getCenter() const { return (m_xyz1 + m_xyz2) / 2.f; }
    Vec3<float> getDelta() const { return m_xyz2 - m_xyz1; }
    float getArea() const {
        Vec3<float> xyz = m_xyz2 - m_xyz1;
        return xyz.x * xyz.y + xyz.x * xyz.z + xyz.y * xyz.z;
    }
    std::pair<Vec3<float>, Vec3<float>> getBounds() const { return {m_xyz1, m_xyz2}; }
    void setBounds(const Vec3<float>& xyz1, const Vec3<float>& xyz2) {
        m_xyz1 = xyz1;
        m_xyz2 = xyz2;
    }

   protected:
    Vec3<float> m_xyz1 = Vec3<float>{INFINITY, INFINITY, INFINITY};
    Vec3<float> m_xyz2 = Vec3<float>{-INFINITY, -INFINITY, -INFINITY};
};
} // namespace spt

#endif