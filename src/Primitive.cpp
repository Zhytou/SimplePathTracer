#include "Primitive.hpp"

namespace spt {

bool Primitive::intersect(const Ray& ray, Intersection& its) const {
    // 0. Fall back to default hit test if transform matrix is identity
    if (m_is_identity) {
        bool hit = m_shape->intersect(ray, its);
        its.id   = hit ? m_id : -1;
        return hit;
    }

    // 1. Convert ray to local space
    Vec3f org_world = ray.getOrigin();
    Vec3f dir_world = ray.getDirection(); // normalized direction vector
    Vec3f org_local = Vec3f(m_inv_transform * Vec4f(org_world, 1.0));
    Vec3f dir_local = Vec3f(m_inv_transform * Vec4f(dir_world, 0.0));

    // 2. Calculate the scale factor between distance in world space and distance in local space
    float tmin  = ray.getTMin();
    float tmax  = ray.getTMax();
    float scale = length(dir_local);

    // 2. Do hit test in local space
    Ray ray_local(org_local, dir_local / scale, tmin * scale, tmax * scale);
    bool hit = m_shape->intersect(ray_local, its);

    // 3. Convert hit info into world space
    if (hit) {
        its.id = m_id;
        its.distance /= scale;
        its.point     = Vec3f(m_transform * Vec4f(its.point, 1.f));
        its.normal    = normalize(m_n_transform * its.normal);
        its.tangent   = normalize(Vec3f(m_transform * Vec4f(its.tangent, 0.f)));
        its.bitangent = normalize(cross(its.normal, its.tangent));
        // its.bitangent = normalize(m_transform * Vec4f(its.bitangent, 0.f));
    }

    return hit;
}

AABB Primitive::wrap() const {
    // 0. Fall back to default bounding box if transform matrix is identity
    if (m_is_identity) {
        return m_shape->wrap();
    }

    // 1. Get local space bounding box
    AABB aabb_local  = m_shape->wrap();
    Vec3f xyz1_local = aabb_local.getXYZ1();
    Vec3f xyz2_local = aabb_local.getXYZ2();

    // 2. Generate eight corner vertices of local AABB
    std::array<Vec3f, 8> xyzs_local = {
        Vec3f(xyz1_local.x, xyz1_local.y, xyz1_local.z),
        Vec3f(xyz2_local.x, xyz1_local.y, xyz1_local.z),
        Vec3f(xyz1_local.x, xyz2_local.y, xyz1_local.z),
        Vec3f(xyz1_local.x, xyz1_local.y, xyz2_local.z),

        Vec3f(xyz2_local.x, xyz2_local.y, xyz2_local.z),
        Vec3f(xyz1_local.x, xyz2_local.y, xyz2_local.z),
        Vec3f(xyz2_local.x, xyz1_local.y, xyz2_local.z),
        Vec3f(xyz2_local.x, xyz2_local.y, xyz1_local.z),
    };

    // 3. Convert all vertices to world space and compute new AABB bounds
    Vec3f xyz1_world(INF), xyz2_world(-INF);
    for (auto& xyz_local : xyzs_local) {
        auto xyz_world = Vec3f(m_transform * Vec4f(xyz_local, 1.f));
        xyz1_world     = min(xyz1_world, xyz_world);
        xyz2_world     = max(xyz2_world, xyz_world);
    }

    AABB aabb_world(xyz1_world, xyz2_world);
    return aabb_world;
}

void Primitive::sample(Vec3f& point, Vec3f& normal) const {
    if (m_is_identity) {
        m_shape->sample(point, normal);
        return;
    }

    Vec3f point_local, normal_local;
    m_shape->sample(point_local, normal_local);

    point  = Vec3f(m_transform * Vec4f(point_local, 1.f));
    normal = normalize(m_n_transform * normal_local);
}

} // namespace spt