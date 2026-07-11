#ifndef SPT_RENDERABLE_HPP
#define SPT_RENDERABLE_HPP

#include "Hittable.hpp"
#include "Material.hpp"

namespace spt {

class Renderable : public Hittable {
   public:
    Renderable(int id = -1) : Hittable(id) {}
    virtual ~Renderable() {}

    virtual float getArea() const              = 0;
    virtual Vec3<float> getRandomPoint() const = 0;

    std::shared_ptr<Material> getMaterial() const { return m_material; }
    int getMaterialID() const { return m_matid; }
    void setMaterial(std::shared_ptr<Material> m) { m_material = m; }
    void setMaterialID(int id) { m_matid = id; }

   protected:
    int m_matid = -1;
    std::shared_ptr<Material> m_material;
};

} // namespace spt

#endif
