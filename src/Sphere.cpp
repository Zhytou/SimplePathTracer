#include "Sphere.hpp"

namespace spt {

bool Sphere::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    // ===================================================================================
    // [Mathematical Derivation: Ray-Sphere Intersection Algorithm]
    //
    // 1. Core Equation (Substituting Ray Equation into Sphere Equation):
    // Ray: P(t) = O + t*D
    // Sphere Surface: (P - C) · (P - C) = r^2
    // Where O = ray origin, D = ray direction, C = sphere center, r = sphere radius.
    //
    // Substituting P(t) into the sphere equation gives:
    // (O + t*D - C) · (O + t*D - C) = r^2
    // Let OC = O - C (Vector from sphere center to ray origin):
    // (OC + t*D) · (OC + t*D) = r^2
    //
    // 2. Quadratic Form (At^2 + Bt + C_constant = 0):
    // Expanding the dot product:
    // (D · D)*t^2 + 2*(OC · D)*t + (OC · OC - r^2) = 0
    //
    // 3. Solving via Simplified Quadratic Formula (Using h = B/2):
    // To minimize multiplications, let h = B/2 = OC · D.
    // The coefficients become:
    // a = D · D            (Squared magnitude of the ray direction)
    // h = OC · D           (Projection of OC onto the ray direction)
    // c = OC · OC - r^2    (Squared distance from origin to center minus squared radius)
    //
    // Simplified Discriminant (det' = h^2 - a*c):
    // - det' < 0 : No real roots (Ray misses the sphere).
    // - det' = 0 : One real root (Ray is perfectly tangent to the sphere).
    // - det' > 0 : Two real roots (Ray penetrates the sphere).
    //
    // Solving for t (root = (-h ± sqrt(det')) / a):
    // - We first test the smaller root (-h - sqrt(det')) for the closest valid hit.
    // - If it falls outside [tmin, tmax], we test the larger root (-h + sqrt(det')).
    //
    // 4. Variable Meanings:
    // - a            : Represents the geometric scale of the ray direction vector.
    // - h            : Determines if the sphere center lies ahead or behind the ray origin.
    // - c            : Represents the positional relationship (c < 0 means origin is inside).
    // - discriminant : Geometric indicator of the intersection state.
    // ===================================================================================

    Vec3<float> origin    = ray.getOrigin();
    Vec3<float> direction = ray.getDirection();

    float a = dot(direction, direction);
    float h = dot(origin - m_center, direction);
    float c = dot(origin - m_center, origin - m_center) - m_radius * m_radius;

    float discriminant = h * h - a * c; // discriminant of quadratic equation
    if (discriminant < 0.0f) {
        return false;
    }

    float sqrt = std::sqrt(discriminant);
    float t1   = (-h - sqrt) / a;
    float t2   = (-h + sqrt) / a;
    if ((t1 < tmin || t1 > tmax) && (t2 < tmin || t2 > tmax)) { // both roots are outside the valid range
        return false;
    }

    float t       = t1 >= tmin && t1 <= tmax ? t1 : t2;
    Vec3<float> p = ray.eval(t);
    rec.distance  = t;
    rec.point     = p;
    rec.texcoord  = parameterize(p);
    rec.normal    = normalize(p - m_center);

    return true;
}

AABB Sphere::wrap() const {
    Vec3<float> xyz1(m_center.x - m_radius, m_center.y - m_radius, m_center.z - m_radius);
    Vec3<float> xyz2(m_center.x + m_radius, m_center.y + m_radius, m_center.z + m_radius);
    return AABB(xyz1, xyz2);
}

Vec3<float> Sphere::sample() const {
    float phi   = rand(0.0f, PI) - PI * 0.5f;
    float theta = rand(0.0f, 2 * PI);

    float x = ::cos(phi) * ::cos(theta);
    float y = ::cos(phi) * ::sin(theta);
    float z = ::sin(phi);

    return m_center + Vec3<float>(x, y, z) * m_radius;
}

Vec2<float> Sphere::parameterize(const Vec3<float>& point) const {
    Vec3<float> dir = normalize(point - m_center);
    float phi       = std::acos(dir.y);
    float theta     = std::atan2(dir.z, dir.x) + PI;

    float u = theta / (2 * PI);
    float v = phi / PI;

    return Vec2<float>(u, v);
}

} // namespace spt
