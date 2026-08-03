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
    float getIOR() const { return m_ior; }
    Vec3<float> getAbsorption() const { return m_absorption; }

    void setID(int id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }
    void setIOR(float ior) { m_ior = ior; }
    void setAbsorption(const Vec3<float>& absorption) { m_absorption = absorption; }

   private:
    int m_id = -1;
    std::string m_name;

    float m_ior              = 1.0f;
    Vec3<float> m_absorption = Vec3<float>(1.0f);
};

} // namespace spt

#endif
