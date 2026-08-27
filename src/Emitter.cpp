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
    float cos_theta       = std::max(dir_local.z, 0.f);  // only consider forward direction

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

std::shared_ptr<DES> DES::create(const std::vector<std::shared_ptr<Emitter>>& emitters) {
    auto des = std::make_shared<DES>();
    for (auto emitter : emitters) {
        if (emitter->isDelta()) {
            des->m_delta_emitters.push_back(emitter);
        } else {
            des->m_areas.push_back(emitter->getPrimitive()->getShape()->area());
            des->m_nondelta_emitters.push_back(emitter);
        }
    }
    return des;
}

std::shared_ptr<Emitter> DES::sample() const {
    float p   = rand(0.f, 1.f);
    int n1    = m_delta_emitters.size();
    int n2    = m_nondelta_emitters.size();
    float ref = 1.f * n1 / (n1 + n2);
    if (p < ref) {
        int size = m_delta_emitters.size();
        UniformDistribution1D<int> dist(0, size - 1);
        return m_delta_emitters[dist.sample()];
    } else {
        int size = m_nondelta_emitters.size();
        PrefixDiscreteDistribution1D dist(m_areas);
        return m_nondelta_emitters[dist.sample()];
    }
}

float DES::prob(std::shared_ptr<Emitter> emitter) const {
    if (emitter == nullptr) { return 0.f; }

    if (emitter->isDelta()) {
        int size  = m_delta_emitters.size();
        int index = -1;
        for (int i = 0; i < size; i++) {
            if (m_delta_emitters[i] == emitter) {
                index = i;
                break;
            }
        }
        UniformDistribution1D<int> dist(0, size - 1);
        return dist.pdf(index);
    } else {
        int size  = m_nondelta_emitters.size();
        int index = -1;
        for (int i = 0; i < size; i++) {
            if (m_nondelta_emitters[i] == emitter) {
                index = i;
                break;
            }
        }
        PrefixDiscreteDistribution1D dist(m_areas);
        return dist.pdf(index);
    }
}

} // namespace spt