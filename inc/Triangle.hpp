#ifndef SPT_TRIANGLE_HPP
#define SPT_TRIANGLE_HPP

#include "Shape.hpp"

namespace spt {

class Triangle : public Shape {
   public:
    Triangle(int id) : Shape(id) {}
    Triangle(int id, const std::array<Vec3f, 3>& v, const std::array<Vec3f, 3>& n, const std::array<Vec2f, 3>& uv) : Shape(id), m_vertex(v), m_normal(n), m_texcoord(uv) {}
    ~Triangle() {}

    virtual const char* getTypeName() const override { return "Triangle"; }
    const std::array<Vec3f, 3>& getVertex() const { return m_vertex; }
    const std::array<Vec2f, 3>& getTexCoord() const { return m_texcoord; }
    const std::array<Vec3f, 3>& getNormal() const { return m_normal; }
    void setVertex(const std::array<Vec3f, 3>& v) { m_vertex = v; }
    void setTexCoord(const std::array<Vec2f, 3>& uv) { m_texcoord = uv; }
    void setNormal(const std::array<Vec3f, 3>& n) { m_normal = n; }

    virtual bool intersect(const Ray& ray, Intersection& its) const override;
    virtual AABB wrap() const override;
    virtual void sample(Vec3f& p, Vec3f& n) const override;
    virtual Vec2f parameterize(const Vec3f& p) const override;
    virtual float area() const override { return length(cross(m_vertex[2] - m_vertex[0], m_vertex[1] - m_vertex[0])) / 2; }
    virtual Vec3f center() const override { return (m_vertex[0] + m_vertex[1] + m_vertex[2]) / 3.f; }

   private:
    std::array<Vec3f, 3> m_vertex;
    std::array<Vec2f, 3> m_texcoord;
    std::array<Vec3f, 3> m_normal;
};

} // namespace spt

#endif