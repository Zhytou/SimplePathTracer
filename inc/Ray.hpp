#ifndef SPT_RAY_HPP
#define SPT_RAY_HPP

#include "Utils.hpp"

namespace spt {
class Ray {
   public:
    Ray()  = default;
    ~Ray() = default;
    Ray(const Vec3<float>& org, const Vec3<float>& dir) : m_origin(org), m_direction(normalize(dir)) {}

    Vec3<float> getOrigin() const { return m_origin; }
    Vec3<float> getDirection() const { return m_direction; }
    Vec3<float> getPointAt(const float& t) const { return m_origin + m_direction * t; }
    void setOrigin(const Vec3<float>& org) { m_origin = org; }
    void setDirection(const Vec3<float>& dir) { m_direction = normalize(dir); }

   private:
    Vec3<float> m_origin;
    Vec3<float> m_direction;
};

} // namespace spt

#endif