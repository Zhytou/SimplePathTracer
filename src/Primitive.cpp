#include "Primitive.hpp"

namespace spt {

bool Primitive::intersect(const Ray& ray, Intersection& its) const {
    // 0. Fall back to default hit test if transform matrix is identity
    if (m_is_identity) {
        bool hit = m_shape->intersect(ray, its);
        its.id   = hit ? getID() : -1;
        return hit;
    }

    // 1. Convert ray to local space
    Vec3<float> org_world  = ray.getOrigin();
    Vec3<float> dir_world  = ray.getDirection(); // normalized direction vector
    Vec4<float> org_local4 = m_inv_transform * Vec4<float>(org_world.x, org_world.y, org_world.z, 1.0);
    Vec4<float> dir_local4 = m_inv_transform * Vec4<float>(dir_world.x, dir_world.y, dir_world.z, 0.0);
    Vec3<float> org_local  = Vec3<float>(org_local4.x, org_local4.y, org_local4.z);
    Vec3<float> dir_local  = Vec3<float>(dir_local4.x, dir_local4.y, dir_local4.z);

    // 2. Calculate the scale factor between distance in world space and distance in local space
    float tmin  = ray.getTMin();
    float tmax  = ray.getTMax();
    float scale = length(dir_local);

    // 2. Do hit test in local space
    Ray ray_local(org_local, dir_local / scale, tmin * scale, tmax * scale);
    bool hit = m_shape->intersect(ray_local, its);

    // 3. Convert hit info into world space
    if (hit) {
        Vec4<float> point_world4 = m_transform * Vec4<float>(its.point.x, its.point.y, its.point.z, 1.0);

        its.id       = getID();
        its.distance = its.distance / scale;
        its.point    = Vec3<float>(point_world4.x, point_world4.y, point_world4.z);
        its.normal   = normalize(m_n_transform * its.normal);
    }

    return hit;
}

AABB Primitive::wrap() const {
    // 0. Fall back to default bounding box if transform matrix is identity
    if (m_is_identity) {
        return m_shape->wrap();
    }

    // 1. Get local space bounding box
    AABB aabb_local        = m_shape->wrap();
    Vec3<float> xyz1_local = aabb_local.getXYZ1();
    Vec3<float> xyz2_local = aabb_local.getXYZ2();

    // 2. Generate eight corner vertices of local AABB
    std::array<Vec4<float>, 8> xyzs_local4 = {
        Vec4<float>(xyz1_local.x, xyz1_local.y, xyz1_local.z, 1.0),
        Vec4<float>(xyz2_local.x, xyz1_local.y, xyz1_local.z, 1.0),
        Vec4<float>(xyz1_local.x, xyz2_local.y, xyz1_local.z, 1.0),
        Vec4<float>(xyz1_local.x, xyz1_local.y, xyz2_local.z, 1.0),

        Vec4<float>(xyz2_local.x, xyz2_local.y, xyz2_local.z, 1.0),
        Vec4<float>(xyz1_local.x, xyz2_local.y, xyz2_local.z, 1.0),
        Vec4<float>(xyz2_local.x, xyz1_local.y, xyz2_local.z, 1.0),
        Vec4<float>(xyz2_local.x, xyz2_local.y, xyz1_local.z, 1.0),
    };

    // 3. Convert all vertices to world space and compute new AABB bounds
    Vec4<float> xyz1_world4(INFINITY), xyz2_world4(-INFINITY);
    for (auto& xyz_local4 : xyzs_local4) {
        auto xyz_world4 = m_transform * xyz_local4;
        xyz1_world4     = min(xyz1_world4, xyz_world4);
        xyz2_world4     = max(xyz2_world4, xyz_world4);
    }

    AABB aabb_world = AABB({xyz1_world4.x, xyz1_world4.y, xyz1_world4.z}, {xyz2_world4.x, xyz2_world4.y, xyz2_world4.z});
    return aabb_world;
}

Vec3<float> Primitive::sample() const {
    if (m_is_identity) {
        return m_shape->sample();
    }

    auto point_local = m_shape->sample();
    auto point_world = m_transform * Vec4<float>(point_local.x, point_local.y, point_local.z, 1.0);
    return {point_world.x, point_world.y, point_world.z};
}

} // namespace spt