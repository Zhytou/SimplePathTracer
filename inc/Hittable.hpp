#ifndef SPT_HITTABLE_HPP
#define SPT_HITTABLE_HPP

#include "AABB.hpp"
#include "Material.hpp"
#include "Ray.hpp"
#include "Utils.hpp"

namespace spt {

struct HitRecord {
    int id                             = -1;
    float distance                     = INFINITY;
    Vec3<float> point                  = Vec3<float>{0.f, 0.f, 0.f};
    Vec2<float> texcoord               = Vec2<float>{0.f, 0.f};
    Vec3<float> normal                 = Vec3<float>{0.f, 0.f, 0.f};
    std::shared_ptr<Material> material = nullptr;
};

class Hittable {
   public:
    Hittable(int id = -1) : m_id(id) {}
    virtual ~Hittable() {}

    int getID() const { return m_id; }
    void setID(int id) { m_id = id; }

    virtual bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const = 0;
    virtual AABB wrap() const                                                      = 0;

   protected:
    int m_id;
};

} // namespace spt

#endif