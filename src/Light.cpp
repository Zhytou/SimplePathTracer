#include "Light.hpp"

namespace spt {

Vec3<float> AreaLight::sample(const Vec3<float>& p) const {
    if (m_object == nullptr) { throw std::runtime_error("AreaLight::sample: no light source!"); } // No light source

    Vec3<float> pp = m_object->getRandomPoint();
    return normalize(pp - p);
}

float AreaLight::pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const {
    if (m_object == nullptr) { throw std::runtime_error("AreaLight::pdf: no light source!"); } // No light source

    float area      = m_object->getArea();
    float cos_light = std::fabs(dot(n, wo));
    if (area <= EPS || cos_light < EPS) { return 0.f; } // Downgrade to delta light

    float pdf  = 1.f / area;
    float dis2 = dis * dis;

    return pdf * dis2 / cos_light;
}

} // namespace spt