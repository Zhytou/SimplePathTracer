#ifndef SPT_DES_HPP
#define SPT_DES_HPP

#include <vector>

#include "Emitter.hpp"
#include "Utils.hpp"

namespace spt {

/**
 * @brief Direct Emission Sampler (DES)
 */
class DES {
   public:
    /**
     * @brief Constructs a Direct Emission Sampler from a collection of scene emitters.
     * 
     * @param emitters Vector of emitters (light sources) to be sampled.
     * @return Shared pointer to the newly created DES instance.
     */
    static std::shared_ptr<DES> create(std::vector<std::shared_ptr<Emitter>> emitters);
    /**
     * @brief Sample an emitter from the collection of emitters.
     * 
     * @param delta If true, sample from delta light sources (e.g., directional/point lights);
     *              otherwise, sample from area light sources.
     * @return The selected emitter and corresponding probability.
    */
    std::pair<std::shared_ptr<Emitter>, float> sample(bool delta = false) const;
    /**
     * @brief Calculate the selection probability of a emitter.
     */
    float prob(std::shared_ptr<Emitter>) const;

   private:
    std::vector<std::shared_ptr<Emitter>> m_emitters;
    std::vector<float> m_areas;
};

} // namespace spt

#endif