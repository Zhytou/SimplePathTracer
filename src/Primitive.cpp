#include "Primitive.hpp"

namespace spt {

bool Primitive::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    // TODO: add world space transform
    bool hit = m_shape->hit(ray, tmin, tmax, rec);
    rec.id   = hit ? getID() : -1;

    return hit;
}

AABB Primitive::wrap() const {
    // TODO: add world space transform
    return m_shape->wrap();
}

Vec3<float> Primitive::sample() const {
    // TODO: add world space transform
    return m_shape->sample();
}

} // namespace spt