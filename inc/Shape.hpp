#ifndef SPT_SHAPE_HPP
#define SPT_SHAPE_HPP

#include "AABB.hpp"
#include "Intersection.hpp"
#include "Material.hpp"
#include "Ray.hpp"
#include "Utils.hpp"

namespace spt {

class Shape {
   public:
    Shape(int id = -1) : m_id(id) {}
    virtual ~Shape() {}

    int getID() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void setID(int id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Test ray intersection against the shape under local coordinates.
     * 
     * @param ray Ray defined in local coordinate space for intersection test.
     * @param its Intersection info struct.
     * @return True if intersection exists; false otherwise.
     */
    virtual bool intersect(const Ray& ray, Intersection& its) const = 0;
    /**
     * @brief Get the local-space bounding box of the shape.
     * 
     * @return Local-space bounding box of the shape.
     */
    virtual AABB wrap() const = 0;
    /**
     * @brief Sample a random point on the shape.
     * 
     * @param[out] p Random point on the shape.
     * @param[out] n Normal at the point on the shape.
     */
    virtual void sample(Vec3<float>& p, Vec3<float>& n) const = 0;
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
    std::string m_name;
};

} // namespace spt

#endif