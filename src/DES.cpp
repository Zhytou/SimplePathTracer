#include "DES.hpp"

namespace spt {

void DES::init(std::vector<std::shared_ptr<Emitter>> emitters) {
    for (auto emitter : emitters) {
        if (emitter->isDelta()) { continue; }
        m_emitters.push_back(emitter);
        m_areas.push_back((m_areas.empty() ? 0.f : m_areas.back()) + emitter->getArea());
    }
    for (auto emitter : emitters) {
        if (!emitter->isDelta()) { continue; }
        m_emitters.push_back(emitter);
    } // make non-delta emitters first and delta emitters last
}

std::pair<std::shared_ptr<Emitter>, float> DES::sample(bool delta) const {
    if (m_emitters.empty()) { return {nullptr, 0.f}; }

    if (delta) { // uniform sampling for delta emitters
        int i = rand(m_areas.size(), m_emitters.size() - 1);
        return std::make_pair(m_emitters[i], 1 / (m_emitters.size() - m_areas.size()));
    } else { // importance sampling based on area for non-delta emitters
        float x  = rand(0.f, m_areas.back());
        auto itr = std::lower_bound(m_areas.begin(), m_areas.end(), x);
        int i    = std::distance(m_areas.begin(), itr);
        return std::make_pair(m_emitters[i], m_areas[i] / m_areas.back());
    }
}
} // namespace spt
