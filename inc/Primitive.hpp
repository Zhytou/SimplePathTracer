#ifndef SPT_PRIMITIVE_HPP
#define SPT_PRIMITIVE_HPP

#include "Emitter.hpp"
#include "HitRecord.hpp"
#include "Material.hpp"
#include "Shape.hpp"
#include "Transform.hpp"

namespace spt {

class Primitive {
   public:
    Primitive()  = default;
    ~Primitive() = default;
    Primitive(int id, std::shared_ptr<Shape> shape, std::shared_ptr<Material> material) : m_id(id), m_shape(shape), m_material(material) {}

    int getID() const { return m_id; }
    std::shared_ptr<Shape> getShape() const { return m_shape; }
    std::shared_ptr<Material> getMaterial() const { return m_material; }
    std::shared_ptr<AreaEmitter> getEmitter() const { return m_emitter; }
    void setShape(std::shared_ptr<Shape> shape) { m_shape = shape; }
    void setMaterial(std::shared_ptr<Material> material) { m_material = material; }
    void setEmitter(std::shared_ptr<AreaEmitter> emitter) { m_emitter = emitter; }

    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const;
    AABB wrap() const;
    Vec3<float> sample() const;

   private:
    int m_id = -1;

    std::shared_ptr<Shape> m_shape         = nullptr;
    std::shared_ptr<Material> m_material   = nullptr;
    std::shared_ptr<AreaEmitter> m_emitter = nullptr;

    Transform m_transform;
};

} // namespace spt

#endif