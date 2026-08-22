#include "Emitter.hpp"
#include "Distribution.hpp"
#include "Primitive.hpp"
#include "Ray.hpp"

namespace spt {

Vec3<float> AreaEmitter::sample(const Vec3<float>& point_ref, Vec3<float>& point, Vec3<float>& normal) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::sample: invalid primitive!"); } // No light source

    auto prm = m_primitive.lock();
    auto spe = prm->getShape();
    prm->sample(point, normal);

    float area = spe->area();
    if (area <= EPS) { throw std::runtime_error(std::format("AreaEmitter::sample: invalid area {}!", area)); }

    Vec3f tangent, bitangent;
    TBN(normal, tangent, bitangent);
    Vec3f radiance = le(toLocal(point_ref - point, tangent, bitangent, normal));

    return radiance * area; // radiance / pdf_area
}

Vec3<float> AreaEmitter::sample(const Distribution& distribution, Ray& ray, Vec3<float>& normal) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::sample: invalid primitive!"); } // No light source

    Vec3<float> origin, direction;
    auto prm = m_primitive.lock();
    auto spe = prm->getShape();
    prm->sample(origin, normal);

    float area = spe->area();
    if (area <= EPS) { throw std::runtime_error(std::format("AreaEmitter::sample: invalid area {}!", area)); }
    float pdf_a = 1.f / area; // pdf_area

    Vec3<float> direction_local = distribution.sample();
    float pdf_w                 = distribution.pdf(direction_local);
    float cos_theta             = direction_local.z;

    Vec3<float> tangent, bitangent;
    TBN(normal, tangent, bitangent);
    direction = toWorld(direction_local, tangent, bitangent, normal);

    ray = Ray(origin, direction);
    return le(direction_local) * cos_theta / (pdf_a * pdf_w);
}

float AreaEmitter::pdf() const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::pdf: invalid primitive!"); } // No light source

    auto prm   = m_primitive.lock();
    auto spe   = prm->getShape();
    float area = spe->area();
    if (area <= EPS) { throw std::runtime_error(std::format("AreaEmitter::pdf: invalid area {}!", area)); }

    return 1.f / area; // pdf_area
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

std::shared_ptr<Emitter> DES::sample() const {
    float x  = rand(0.f, m_areas.back());
    auto itr = std::lower_bound(m_areas.begin(), m_areas.end(), x);
    int i    = std::distance(m_areas.begin(), itr);
    return m_emitters[i];
}

float DES::prob(std::shared_ptr<Emitter> emitter) const {
    if (emitter == nullptr || m_emitters.empty()) { return 0.f; }

    return emitter->getPrimitive()->getShape()->area() / m_areas.back();
}

} // namespace spt