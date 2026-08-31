#ifndef SPT_SPHERE_HPP
#define SPT_SPHERE_HPP

#include "Shape.hpp"

namespace spt {

class Sphere : public Shape {
   public:
    Sphere(int id) : Shape(id) {}
    Sphere(int id, const Vec3f& center, float radius) : Shape(id), m_center(center), m_radius(radius) {}
    ~Sphere() {}

    virtual const char* getTypeName() const override { return "Sphere"; }
    const Vec3f& getCenter() const { return m_center; }
    float getRadius() const { return m_radius; }
    void setCenter(const Vec3f& center) { m_center = center; }
    void setRadius(float radius) { m_radius = radius; }

    virtual bool intersect(const Ray& ray, Intersection& its) const override;
    virtual AABB wrap() const override;
    virtual void sample(Vec3f& p, Vec3f& n) const override;
    virtual Vec2f parameterize(const Vec3f& p) const override;
    virtual float area() const override { return 4 * PI * m_radius * m_radius; }
    virtual Vec3f center() const override { return m_center; }

   private:
    Vec3f m_center;
    float m_radius;
};

} // namespace spt

#endif