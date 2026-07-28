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
    DES(std::vector<std::shared_ptr<Emitter>> emitters) { init(emitters); }
    ~DES() {}

    void init(std::vector<std::shared_ptr<Emitter>> emitters);
    std::pair<std::shared_ptr<Emitter>, float> sample(bool delta = false) const;

   private:
    std::vector<std::shared_ptr<Emitter>> m_emitters;
    std::vector<float> m_areas;
};

} // namespace spt

#endif