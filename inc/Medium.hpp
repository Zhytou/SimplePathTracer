#ifndef SPT_MEDIUM_HPP
#define SPT_MEDIUM_HPP

#include "Utils.hpp"

namespace spt {

class Medium {
   public:
    Medium(int id) : m_id(id) {}
    ~Medium() {}

    int getID() const { return m_id; }
    const std::string& getName() const { return m_name; }
    Vec3<float> getAbsorption() const { return m_absorption; }
    Vec3<float> getScattering() const { return m_scattering; }
    Vec3<float> getExtinction() const { return m_extinction; }

    void setID(int id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }
    void setAbsorption(const Vec3<float>& absorption) { m_absorption = absorption; }
    void setScattering(const Vec3<float>& scattering) { m_scattering = scattering; }
    void setExtinction(const Vec3<float>& extinction) { m_extinction = extinction; }

   private:
    int m_id = -1;
    std::string m_name;

    float m_ior = 1.0f;
    Vec3<float> m_absorption;
    Vec3<float> m_scattering;
    Vec3<float> m_extinction;
};

} // namespace spt

#endif
