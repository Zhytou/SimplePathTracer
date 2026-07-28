#ifndef SPT_TRIANGLE_HPP
#define SPT_TRIANGLE_HPP

#include "Shape.hpp"

#include <array>
#include <iostream>
#include <memory>

namespace spt {

class Triangle : public Shape {
   public:
    Triangle(int id) : Shape(id) {}
    ~Triangle() {}

    float getArea() const override { return length(cross(m_vertex[2] - m_vertex[0], m_vertex[1] - m_vertex[0])) / 2; }
    void setVertex(const std::array<Vec3<float>, 3>& v) { m_vertex = v; }
    void setTexCoord(const std::array<Vec2<float>, 3>& uv) { m_texcoord = uv; }
    void setNormal(const Vec3<float>& n) { m_normal = n; }

    virtual bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override;
    virtual AABB wrap() const override;
    Vec3<float> sample() const override;

   private:
    std::array<Vec3<float>, 3> m_vertex;
    std::array<Vec2<float>, 3> m_texcoord;
    Vec3<float> m_normal;
};

} // namespace spt

#endif