#ifndef SPT_TRIANGLE_HPP
#define SPT_TRIANGLE_HPP

#include "Hittable.hpp"
#include "Material.hpp"

#include <array>
#include <iostream>
#include <memory>

namespace spt {

class Triangle : public Hittable {
   public:
    Triangle(int id) : Hittable(id) {}
    ~Triangle() {}

    Vec3<float> getNormal() const { return m_normal; }
    std::shared_ptr<Material> getMaterial() const { return m_material; }
    int getMaterialID() const { return m_matid; }
    float getArea() const { return cross(m_vertex[2] - m_vertex[0], m_vertex[1] - m_vertex[0]).length() / 2; }
    Vec3<float> getRandomPoint() const;
    void setVertex(const std::array<Vec3<float>, 3>& v) { m_vertex = v; }
    void setTexCoord(const std::array<Vec2<float>, 3>& uv) { m_texcoord = uv; }
    void setNormal(const Vec3<float>& n) { m_normal = n; }
    void setMaterial(std::shared_ptr<Material> m) { m_material = m; }
    void setMaterialID(int id) { m_matid = id; }

    virtual bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override;
    virtual AABB wrap() const override;

   private:
    std::array<Vec3<float>, 3> m_vertex;
    std::array<Vec2<float>, 3> m_texcoord;
    Vec3<float> m_normal;
    std::shared_ptr<Material> m_material;
    int m_matid = -1;
};

} // namespace spt

#endif