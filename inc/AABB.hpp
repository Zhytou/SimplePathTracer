#ifndef SPT_AABB_HPP
#define SPT_AABB_HPP

#include <cassert>
#include <cfloat>
#include <memory>
#include <vector>

#include "Ray.hpp"
#include "Utils.hpp"

namespace spt {

class Primitive;

/**
 * @brief Axis-Aligned Bounding Box
 */
class AABB {
   public:
    AABB(const Vec3<float>& xyz1 = Vec3<float>(INFINITY), const Vec3<float>& xyz2 = Vec3<float>(-INFINITY)) : m_xyz1(xyz1), m_xyz2(xyz2) {}

    const Vec3<float>& getXYZ1() const { return m_xyz1; }
    const Vec3<float>& getXYZ2() const { return m_xyz2; }
    void setXYZ1(const Vec3<float>& xyz1) { m_xyz1 = xyz1; }
    void setXYZ2(const Vec3<float>& xyz2) { m_xyz2 = xyz2; }

    bool operator==(const AABB& other) const { return m_xyz1 == other.m_xyz1 && m_xyz2 == other.m_xyz2; }
    bool operator!=(const AABB& other) const { return m_xyz1 != other.m_xyz1 || m_xyz2 != other.m_xyz2; }

    static AABB create(const std::vector<std::shared_ptr<Primitive>>& primitives);
    bool intersect(const Ray& ray) const;
    AABB& merge(const AABB& other);

    Vec3<float> center() const { return (m_xyz1 + m_xyz2) / 2.f; }
    Vec3<float> extent() const { return m_xyz2 - m_xyz1; }
    float area() const {
        Vec3<float> xyz = m_xyz2 - m_xyz1;
        return xyz.x * xyz.y + xyz.x * xyz.z + xyz.y * xyz.z;
    }

   private:
    Vec3<float> m_xyz1 = Vec3<float>(INFINITY);
    Vec3<float> m_xyz2 = Vec3<float>(-INFINITY);
};

} // namespace spt

#endif