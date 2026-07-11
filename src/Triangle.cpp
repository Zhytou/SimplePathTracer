#include "Triangle.hpp"
#include "Material.hpp"

#include <cassert>

namespace spt {

Vec3<float> Triangle::getRandomPoint() const {
    Vec3<float> e1 = m_vertex[2] - m_vertex[0];
    Vec3<float> e2 = m_vertex[1] - m_vertex[0];

    float r1 = rand(1.f);
    float r2 = rand(1.f);

    float u = 1 - std::sqrt(r1);
    float v = r2 * std::sqrt(r1);

    // p = (1 - u - v) * p0 + u * p1 + v * p2 = p0 + u * e1 + v * e2
    // u >= 0, v >= 0, u + v <= 1
    return e1 * u + e2 * v + m_vertex[0];
}

bool Triangle::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    // ===================================================================================
    // [Mathematical Derivation: Möller–Trumbore Intersection Algorithm]
    //
    // 1. Core Equation (Equating Ray and Barycentric Equations):
    // Ray: P(t) = O + t*D
    // Triangle Surface: P(u,v) = (1-u-v)*V0 + u*V1 + v*V2 = V0 + u*E1 + v*E2
    // Where E1 = V1 - V0, and E2 = V2 - V0.
    //
    // Equating them gives: O + t*D = V0 + u*E1 + v*E2
    // Moving unknowns to the left side: -t*D + u*E1 + v*E2 = O - V0
    //
    // 2. Matrix Form (Ax = B):
    // [ -D,  E1,  E2 ] * [ t,  u,  v ]^T = T_vec   (Where T_vec = O - V0)
    //
    // 3. Solving via Cramer's Rule:
    // We use the scalar triple product identity: det(A, B, C) = (A x B) · C
    //
    // det   = det(-D, E1, E2) = (D x E2) · E1       ==> Let P_vec = D x E2, det = E1 · P_vec
    // u_num = det(T_vec, E1, E2) = (T_vec x D) · E2 ==> u = (T_vec · P_vec) / det
    // v_num = det(-D, E1, T_vec) = (T_vec x E1) · D ==> Let Q_vec = T_vec x E1, v = D · Q_vec / det
    // t_num = det(-D, E1, T_vec) = (T_vec x E1) · E2 ==> t = (E2 · Q_vec) / det
    //
    // 4. Variable Meanings:
    // - E1, E2 : Edge vectors of the triangle.
    // - T_vec  : Vector from V0 to the ray origin (Translation vector).
    // - P_vec  : Vector perpendicular to both the ray direction and edge E2.
    // - Q_vec  : Vector perpendicular to both T_vec and edge E1.
    // ===================================================================================

    Vec3<float> origin    = ray.getOrigin();
    Vec3<float> direction = ray.getDirection();

    Vec3<float> e1 = m_vertex[2] - m_vertex[0];
    Vec3<float> e2 = m_vertex[1] - m_vertex[0];

    // 1. Calculate the determinant (det) of matrix [-D, E1, E2]
    Vec3<float> pvec = cross(direction, e2);
    float det        = dot(e1, pvec);

    // 2. Check if the ray is parallel to the triangle plane
    if (std::abs(det) < EPS) {
        return false;
    }
    float invdet = 1.0f / det;

    // 3. Calculate and validate Barycentric coordinate 'u'
    Vec3<float> tvec = origin - m_vertex[0];
    float u          = dot(tvec, pvec) * invdet;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    // 4. Calculate and validate Barycentric coordinate 'v'
    Vec3<float> qvec = cross(tvec, e1);
    float v          = dot(direction, qvec) * invdet;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // 5. Calculate the ray parameter 't' (distance)
    float t = dot(e2, qvec) * invdet;
    if (t < tmin || t > tmax) {
        return false;
    }

    // 6. Interpolate texture coordinates using weights: w = 1 - u - v
    float w              = 1.0f - u - v;
    Vec2<float> texcoord = m_texcoord[0] * w + m_texcoord[1] * u + m_texcoord[2] * v;

    // 7. UV Wrap Mode: Repeat logic
    texcoord.u = std::fmod(texcoord.u, 1.0f);
    texcoord.v = std::fmod(texcoord.v, 1.0f);
    if (texcoord.u < 0) { texcoord.u += 1.0f; }
    if (texcoord.v < 0) { texcoord.v += 1.0f; }

    // 8. Set hit record
    rec.id       = m_id;
    rec.distance = t;
    rec.point    = ray.getPointAt(t);
    rec.texcoord = texcoord;
    rec.normal   = m_normal;
    rec.material = m_material;

    return true;
}

AABB Triangle::wrap() const {
    Vec3<float> xyz1, xyz2;
    xyz1.x = std::min(m_vertex[0].x, std::min(m_vertex[1].x, m_vertex[2].x));
    xyz1.y = std::min(m_vertex[0].y, std::min(m_vertex[1].y, m_vertex[2].y));
    xyz1.z = std::min(m_vertex[0].z, std::min(m_vertex[1].z, m_vertex[2].z));
    xyz2.x = std::max(m_vertex[0].x, std::max(m_vertex[1].x, m_vertex[2].x));
    xyz2.y = std::max(m_vertex[0].y, std::max(m_vertex[1].y, m_vertex[2].y));
    xyz2.z = std::max(m_vertex[0].z, std::max(m_vertex[1].z, m_vertex[2].z));
    return AABB(xyz1, xyz2);
}

} // namespace spt
