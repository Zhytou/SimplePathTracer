#include "Emitter.hpp"
#include "Primitive.hpp"

namespace spt {

Vec3<float> AreaEmitter::sample(const Vec3<float>& p) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::sample: invalid primitive!"); } // No light source

    auto prm = m_primitive.lock();
    auto pp  = prm->sample();
    return normalize(pp - p);
}

float AreaEmitter::pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::pdf: invalid primitive!"); } // No light source

    auto prm   = m_primitive.lock();
    float area = prm->getShape()->area();
    float cos  = std::fabs(dot(n, wo));
    if (area <= EPS || cos < EPS) { return 0.f; } // downgrade to delta light

    float pdf = 1.f / area;         // 1 / dA
    return pdf * (dis * dis) / cos; // 1 / dw = 1 / dA * dis * dis / cos
}

} // namespace spt