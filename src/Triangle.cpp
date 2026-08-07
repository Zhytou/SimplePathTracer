#include "Triangle.hpp"
#include "Material.hpp"

#include <cassert>

namespace spt {

Vec3<float> Triangle::sample() const {
    Vec3<float> e1 = m_vertex[2] - m_vertex[0];
    Vec3<float> e2 = m_vertex[1] - m_vertex[0];

    float r1 = rand(0.f, 1.f);
    float r2 = rand(0.f, 1.f);

    float u = 1 - std::sqrt(r1);
    float v = r2 * std::sqrt(r1);

    // p = (1 - u - v) * p0 + u * p1 + v * p2 = p0 + u * e1 + v * e2
    // u >= 0, v >= 0, u + v <= 1
    return e1 * u + e2 * v + m_vertex[0];
}

bool Triangle::intersect(const Ray& ray, Intersection& its) const {
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
    // [ -D,  E1,  E2 ] * [ t,  u,  v ]^T = O - V0
    //
    // 3. Solving via Cramer's Rule:
    // t = det([ O - V0, E1, E2 ]) / det([ -D, E1, E2 ])
    // u = det([ -D, O - V0, E2 ]) / det([ -D, E1, E2 ])
    // v = det([ -D, E1, O - V0 ]) / det([ -D, E1, E2 ])
    //
    // while det([ A, B, C ]) = (A x B) · C = A · (B x C) = B · (C x A)
    //
    // 4. Define several variables for convenience:
    // - O : Org
    // - D : Dir
    // - E1 : V1 - V0
    // - E2 : V2 - V0
    // - T  : O - V0
    // - P  : cross(D, E2)
    // - Q  : cross(T, E1)
    // - DET : det([ -D, E1, E2 ]) = dot(E1, P)
    // - DET_inv : 1.0f / det([ -D, E1, E2 ])
    // ===================================================================================

    Vec3<float> org = ray.getOrigin();
    Vec3<float> dir = ray.getDirection();

    Vec3<float> e1 = m_vertex[1] - m_vertex[0];
    Vec3<float> e2 = m_vertex[2] - m_vertex[0];

    // 1. Define intermediate variables and calculate determinant of matrix [-D, E1, E2]
    Vec3<float> tvec = org - m_vertex[0];
    Vec3<float> pvec = cross(dir, e2);
    Vec3<float> qvec = cross(tvec, e1);
    float det        = dot(e1, pvec);

    // 2. Check if the ray is parallel to the triangle plane
    if (std::fabs(det) < EPS) {
        return false;
    }
    float det_inv = 1.0f / det;

    // 3. Calculate and validate Barycentric coordinate 'u'
    float u = dot(tvec, pvec) * det_inv;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    // 4. Calculate and validate Barycentric coordinate 'v'
    float v = dot(dir, qvec) * det_inv;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // 5. Calculate the ray parameter 't' (distance)
    float t    = dot(e2, qvec) * det_inv;
    float tmin = ray.getTMin();
    float tmax = ray.getTMax();
    if (t < tmin || t > tmax) {
        return false;
    }

    // 6. Interpolate texture coordinates using weights: w = 1 - u - v
    float w              = 1.0f - u - v;
    Vec2<float> texcoord = m_texcoord[0] * w + m_texcoord[1] * u + m_texcoord[2] * v;

    // 7. Set hit record
    its.distance = t;
    its.point    = ray.eval(t);
    its.texcoord = texcoord;
    its.normal   = m_normal;

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

Vec2<float> Triangle::parameterize(const Vec3<float>& point) const {
    Vec3<float> p  = point;
    Vec3<float> v0 = m_vertex[0];
    Vec3<float> v1 = m_vertex[1];
    Vec3<float> v2 = m_vertex[2];

    Vec3<float> e0 = v1 - v0;
    Vec3<float> e1 = v2 - v0;
    Vec3<float> n  = cross(e0, e1);
    float area     = length(n) / 2;
    if (area < EPS) {
        return m_texcoord[0];
    }

    Vec3<float> pv0 = v0 - p;
    Vec3<float> pv1 = v1 - p;
    Vec3<float> pv2 = v2 - p;

    float u = dot(cross(pv2, pv0), m_normal) / dot(n, m_normal);
    float v = dot(cross(pv0, pv1), m_normal) / dot(n, m_normal);
    float w = 1.0f - u - v;

    return m_texcoord[0] * w + m_texcoord[1] * u + m_texcoord[2] * v;
}

} // namespace spt
