#include "Emitter.hpp"
#include "Primitive.hpp"

namespace spt {

Vec3<float> AreaEmitter::sample(const Vec3<float>& point_x, Vec3<float>& point_y, Vec3<float>& normal_y) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::sample: invalid primitive!"); } // No light source

    auto prm = m_primitive.lock();
    auto spe = prm->getShape();
    prm->sample(point_y, normal_y);

    float area = spe->area();
    float dist = length(point_y - point_x);
    float cos  = std::abs(dot(normal_y, (point_y - point_x) / dist));
    if (dist <= EPS) { return Vec3<float>(0.f); }

    return m_radiance * area * cos / (dist * dist);
}

float AreaEmitter::pdf(const Vec3<float>& point_x, const Vec3<float>& point_y, const Vec3<float>& normal_y) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::pdf: invalid primitive!"); } // No light source

    auto prm = m_primitive.lock();
    auto spe = prm->getShape();

    float area = spe->area();
    float dist = length(point_y - point_x);
    float cos  = std::abs(dot(normal_y, (point_y - point_x) / dist));
    if (area <= EPS || cos < EPS) { return 0.f; }

    float pdf_a = 1.f / area;                  // pdf_area
    float pdf_w = pdf_a * (dist * dist) / cos; // pdf_solid_angle
    return pdf_w;
}

std::shared_ptr<DES> DES::create(std::vector<std::shared_ptr<Emitter>> emitters) {
    auto des = std::make_shared<DES>();
    for (auto emitter : emitters) {
        auto area = emitter->getPrimitive()->getShape()->area();
        des->m_emitters.push_back(emitter);
        des->m_areas.push_back((des->m_areas.empty() ? 0.f : des->m_areas.back()) + area);
    }
    return des;
}

std::pair<std::shared_ptr<Emitter>, float> DES::sample() const {
    float x  = rand(0.f, m_areas.back());
    auto itr = std::lower_bound(m_areas.begin(), m_areas.end(), x);
    int i    = std::distance(m_areas.begin(), itr);
    return std::make_pair(m_emitters[i], m_areas[i] / m_areas.back());
}

float DES::prob(std::shared_ptr<Emitter> emitter) const {
    if (emitter == nullptr || m_emitters.empty()) { return 0.f; }

    return emitter->getPrimitive()->getShape()->area() / m_areas.back();
}

} // namespace spt