#ifndef SPT_LIGHT_HPP
#define SPT_LIGHT_HPP

#include <cmath>
#include <map>
#include <memory>
#include <vector>

#include "Renderable.hpp"
#include "Utils.hpp"

namespace spt {

class Light {
   public:
    Light(int id, const Vec3<float>& color) : m_id(id), m_color(color) {}
    virtual ~Light() {}

    virtual bool isDelta() const = 0;
    int getID() const { return m_id; }
    virtual int getObjectID() const { return -1; }
    virtual float getObjectArea() const { return 0.f; }
    Vec3<float> getColor() const { return m_color; }

    virtual Vec3<float> sample(const Vec3<float>& p) const                          = 0;
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const = 0;

   private:
    Vec3<float> m_color = Vec3<float>(0.f);
    int m_id            = -1;
};

class AreaLight : public Light {
   public:
    AreaLight(int id, const Vec3<float>& color, std::shared_ptr<Renderable> object) : Light(id, color), m_object(object) {}
    ~AreaLight() {}

    virtual bool isDelta() const override { return false; }
    virtual int getObjectID() const override { return m_object ? m_object->getID() : -1; }
    virtual float getObjectArea() const override { return m_object ? m_object->getArea() : 0.f; }
    virtual Vec3<float> sample(const Vec3<float>& p) const override;
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override;

   private:
    std::shared_ptr<Renderable> m_object;
};

class PointLight : public Light {
   public:
    PointLight(int id, const Vec3<float>& color, const Vec3<float>& position) : Light(id, color), m_position(position) {}
    ~PointLight() {}

    virtual bool isDelta() const override { return true; }
    virtual Vec3<float> sample(const Vec3<float>& p) const override { return normalize(m_position - p); }
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override { return INFINITY; }

   private:
    Vec3<float> m_position;
};

class DirectionalLight : public Light {
   public:
    DirectionalLight(int id, const Vec3<float>& color, const Vec3<float>& direction) : Light(id, color), m_direction(normalize(direction)) {}
    ~DirectionalLight() {}

    virtual bool isDelta() const override { return true; }
    virtual Vec3<float> sample(const Vec3<float>& p) const override { return -m_direction; }
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override { return INFINITY; }

   private:
    Vec3<float> m_direction;
};

} // namespace spt

#endif