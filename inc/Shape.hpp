#ifndef SPT_SHAPE_HPP
#define SPT_SHAPE_HPP

#include "AABB.hpp"
#include "IntersectRecord.hpp"
#include "Material.hpp"
#include "Ray.hpp"
#include "Utils.hpp"

namespace spt {

class Shape {
   public:
    Shape(int id = -1) : m_id(id) {}
    virtual ~Shape() {}

    int getID() const { return m_id; }
    void setID(int id) { m_id = id; }

    /**
     * @brief Test ray intersection against the shape under local coordinates.
     * 
     * @param ray Ray defined in local coordinate space for intersection test.
     * @param rec Record to store intersection data if hit occurs.
     * @return True if intersection exists; false otherwise.
     */
    virtual bool intersect(const Ray& ray, IntersectRecord& rec) const = 0;
    /**
     * @brief Get the local-space bounding box of the shape.
     * 
     * @return Local-space bounding box of the shape.
     */
    virtual AABB wrap() const = 0;
    /**
     * @brief Sample a random point on the shape.
     * 
     * @return Random point on the shape.
     */
    virtual Vec3<float> sample() const = 0;
    /**
     * @brief Get the texture coordinates of the corresponding point on the shape.
     * 
     * @param p Point on the shape.
     * @return Texture coordinates of the point.
     */
    virtual Vec2<float> parameterize(const Vec3<float>& p) const = 0;
    /**
     * @brief Get the area of the shape.
     * 
     * @return Area of the shape.
     */
    virtual float area() const = 0;
    /**
     * @brief Get the center of the shape.
     * 
     * @return Center of the shape.
     */
    virtual Vec3<float> center() const = 0;

   protected:
    int m_id = -1;
};

} // namespace spt

#endif