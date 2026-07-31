#ifndef SPT_TRANSFORM_HPP
#define SPT_TRANSFORM_HPP

#include "Utils.hpp"

namespace spt {

/**
 * @brief Create translation matrix
 * 
 * @param t Translation offsets along each axis.
 * @return Translation matrix
 */
Mat4x4f translate(const Vec3f& t);

/**
 * @brief Axis-angle rotation
 * 
 * @param r Rotation angles(in degrees) along each axis.
 * @return Rotation matrix
 */
Mat4x4f rotate(const Vec3f& d);

/**
 * @brief Create scale matrix
 * 
 * @param s Scale factors along each axis.
 * @return Scale matrix
 */
Mat4x4f scale(const Vec3f& s);

} // namespace spt

#endif