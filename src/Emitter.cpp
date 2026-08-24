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

    float area = spe->area(); // inv of pdf_pos
    if (area <= EPS) { throw std::runtime_error(std::format("AreaEmitter::sample: invalid area {}!", area)); }

    Vec3f tangent, bitangent;
    TBN(normal, tangent, bitangent);
    Vec3f radiance = le(toLocal(point_ref - point, tangent, bitangent, normal));

    return radiance * area; // radiance / pdf_pos
}

Vec3<float> AreaEmitter::sample(Ray& ray, Vec3<float>& normal) const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::sample: invalid primitive!"); } // No light source

    Vec3<float> origin, direction;
    auto prm = m_primitive.lock();
    auto spe = prm->getShape();
    prm->sample(origin, normal);

    float area = spe->area();
    if (area <= EPS) { throw std::runtime_error(std::format("AreaEmitter::sample: invalid area {}!", area)); }
    float pdf_pos = 1.f / area; // sample position pdf in area

    CosineDistribution distribution;
    Vec3<float> dir_local = distribution.sample();
    float pdf_dir         = distribution.pdf(dir_local); // sample direction pdf in solid angle
    float cos_theta       = dir_local.z;

    Vec3<float> tangent, bitangent;
    TBN(normal, tangent, bitangent);
    direction = toWorld(dir_local, tangent, bitangent, normal);

    ray = Ray(origin, direction);
    return le(dir_local) * cos_theta / (pdf_pos * pdf_dir);
}

float AreaEmitter::pdf() const {
    if (m_primitive.expired()) { throw std::runtime_error("AreaEmitter::pdf: invalid primitive!"); } // No light source

    auto prm   = m_primitive.lock();
    auto spe   = prm->getShape();
    float area = spe->area();
    if (area <= EPS) { throw std::runtime_error(std::format("AreaEmitter::pdf: invalid area {}!", area)); }

    return 1.f / area; // pdf_area
}

float AreaEmitter::pdf(const Vec3<float>& dir_local) const {
    CosineDistribution distribution;
    return distribution.pdf(dir_local);
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