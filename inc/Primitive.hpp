#ifndef SPT_PRIMITIVE_HPP
#define SPT_PRIMITIVE_HPP

#include "Emitter.hpp"
#include "HitRecord.hpp"
#include "Material.hpp"
#include "Shape.hpp"

namespace spt {

class Primitive {
   public:
    Primitive()  = default;
    ~Primitive() = default;
    Primitive(int id, std::shared_ptr<Shape> shape, std::shared_ptr<Material> material, const Mat4x4<float>& transform) : m_id(id), m_shape(shape), m_material(material) { setTransform(transform); }

    int getID() const { return m_id; }
    std::shared_ptr<Shape> getShape() const { return m_shape; }
    std::shared_ptr<Material> getMaterial() const { return m_material; }
    std::shared_ptr<AreaEmitter> getEmitter() const { return m_emitter; }
    const Mat4x4<float>& getTransform() const { return m_transform; }
    const Mat4x4<float>& getInvTransform() const { return m_inv_transform; }

    void setID(int id) { m_id = id; }
    void setShape(std::shared_ptr<Shape> shape) { m_shape = shape; }
    void setMaterial(std::shared_ptr<Material> material) { m_material = material; }
    void setEmitter(std::shared_ptr<AreaEmitter> emitter) { m_emitter = emitter; }
    void setTransform(const Mat4x4<float>& transform) {
        m_is_identity   = transform == Mat4x4<float>::eye();
        m_transform     = transform;
        m_inv_transform = inv_affine(transform);
        m_n_transform   = {
            {transform[0][0], transform[0][1], transform[0][2]},
            {transform[1][0], transform[1][1], transform[1][2]},
            {transform[2][0], transform[2][1], transform[2][2]},
        };
        m_n_transform = transpose(inv(m_n_transform));
    }

    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const;
    AABB wrap() const;
    Vec3<float> sample() const;

   private:
    int m_id = -1;

    std::shared_ptr<Shape> m_shape         = nullptr;
    std::shared_ptr<Material> m_material   = nullptr;
    std::shared_ptr<AreaEmitter> m_emitter = nullptr;

    bool m_is_identity            = true; // whether the transform matrix is identity
    Mat4x4<float> m_transform     = Mat4x4<float>::eye();
    Mat4x4<float> m_inv_transform = Mat4x4<float>::eye();
    Mat3x3<float> m_n_transform   = Mat3x3<float>::eye(); // normal transform matrix
};

} // namespace spt

#endif