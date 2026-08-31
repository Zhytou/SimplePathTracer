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
    Vec3f getAbsorption() const { return m_absorption; }
    Vec3f getScattering() const { return m_scattering; }
    Vec3f getExtinction() const { return m_extinction; }

    void setID(int id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }
    void setAbsorption(const Vec3f& absorption) { m_absorption = absorption; }
    void setScattering(const Vec3f& scattering) { m_scattering = scattering; }
    void setExtinction(const Vec3f& extinction) { m_extinction = extinction; }

   private:
    int m_id = -1;
    std::string m_name;

    float m_ior = 1.0f;
    Vec3f m_absorption;
    Vec3f m_scattering;
    Vec3f m_extinction;
};

} // namespace spt

#endif
