#ifndef SPT_SPHERE_HPP
#define SPT_SPHERE_HPP

#include "Material.hpp"
#include "Renderable.hpp"

#include <array>
#include <iostream>
#include <memory>

namespace spt {

class Sphere : public Renderable {
   public:
    Sphere(int id) : Renderable(id) {}
    ~Sphere() {}

    float getArea() const override { return 4 * PI * m_radius * m_radius; }
    Vec3<float> getRandomPoint() const override;
    Vec2<float> getTexCoord(const Vec3<float>& p) const;
    void setCenter(const Vec3<float>& center) { m_center = center; }
    void setRadius(float radius) { m_radius = radius; }

    virtual bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override;
    virtual AABB wrap() const override;

   private:
    Vec3<float> m_center;
    float m_radius;
};

} // namespace spt

#endif