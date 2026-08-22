#ifndef SPT_RAY_HPP
#define SPT_RAY_HPP

#include "Medium.hpp"
#include "Utils.hpp"

namespace spt {

class Ray {
   public:
    Ray() {}
    Ray(const Vec3<float>& org, const Vec3<float>& dir, float tmin = DIS_EPS, float tmax = INFINITY, std::shared_ptr<Medium> medium = nullptr) : m_origin(org), m_direction(normalize(dir)), m_tmin(tmin), m_tmax(tmax), m_medium(medium) {}

    const Vec3<float>& getOrigin() const { return m_origin; }
    const Vec3<float>& getDirection() const { return m_direction; }
    float getTMin() const { return m_tmin; }
    float getTMax() const { return m_tmax; }
    std::shared_ptr<Medium> getMedium() const { return !m_medium.expired() ? m_medium.lock() : nullptr; }
    void setOrigin(const Vec3<float>& org) { m_origin = org; }
    void setDirection(const Vec3<float>& dir) { m_direction = normalize(dir); }
    void setTMin(float tmin) { m_tmin = tmin; }
    void setTMax(float tmax) { m_tmax = tmax; }
    void setMedium(const std::shared_ptr<Medium>& medium) { m_medium = medium; }

    Vec3<float> eval(const float& t) const {
        if (t < m_tmin || t > m_tmax) { throw std::out_of_range("Ray::eval: t out of range"); }
        return m_origin + m_direction * t;
    }

   private:
    Vec3<float> m_origin;
    Vec3<float> m_direction;

    float m_tmin = DIS_EPS;
    float m_tmax = INFINITY;
    std::weak_ptr<Medium> m_medium;
};

} // namespace spt

#endif