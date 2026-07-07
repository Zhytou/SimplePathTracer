#include "Light.hpp"

namespace spt {
void Light::add(std::shared_ptr<Triangle> triangle) {
    // 1. Get basic info
    auto mtl  = triangle->getMaterial();
    auto name = mtl->getName();
    if (!mtl->isEmissive()) {
        throw std::runtime_error("Light::add: triangle must be emissive!");
    }

    // 2. Add triangle to light list
    m_triangles.push_back(triangle);
    m_psums.push_back(m_sum);
    m_sum += triangle->getArea();
}

std::pair<int, Vec3<float>> Light::sample() const {
    if (m_triangles.empty()) { return {-1, Vec3<float>()}; } // No light source

    int x    = rand(1.0f, 0.0f) * m_sum;
    auto itr = std::lower_bound(m_psums.begin(), m_psums.end(), x);
    int idx  = std::distance(m_psums.begin(), itr);
    idx      = std::min(idx, static_cast<int>(m_triangles.size()) - 1);

    int id            = m_triangles[idx]->getID();
    Vec3<float> point = m_triangles[idx]->getNormal();

    return {id, point};
}

float Light::pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) {
    if (m_sum < EPS || m_triangles.empty()) { return 0.f; }

    float cos_light = std::max(dot(wo, -n), 0.f);
    if (cos_light < EPS) { return 0.f; }

    float pdf  = 1.f / m_sum;
    float dis2 = dis * dis;

    return pdf * dis2 / cos_light;
}

} // namespace spt