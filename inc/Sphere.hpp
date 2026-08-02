#ifndef SPT_SPHERE_HPP
#define SPT_SPHERE_HPP

#include "Shape.hpp"

namespace spt {

class Sphere : public Shape {
   public:
    Sphere(int id) : Shape(id) {}
    Sphere(int id, const Vec3<float>& center, float radius) : Shape(id), m_center(center), m_radius(radius) {}
    ~Sphere() {}

    const Vec3<float>& getCenter() const { return m_center; }
    float getRadius() const { return m_radius; }
    void setCenter(const Vec3<float>& center) { m_center = center; }
    void setRadius(float radius) { m_radius = radius; }

    virtual bool intersect(const Ray& ray, IntersectRecord& rec) const override;
    virtual AABB wrap() const override;
    virtual Vec3<float> sample() const override;
    float area() const override { return 4 * PI * m_radius * m_radius; }
    Vec3<float> center() const override { return m_center; }
    Vec2<float> parameterize(const Vec3<float>& p) const override;

   private:
    Vec3<float> m_center;
    float m_radius;
};

} // namespace spt

#endif