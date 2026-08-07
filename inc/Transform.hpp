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

/**
 * @brief Calculate the direction of reflected light
 * 
 * @param wi Incident direction vector
 * @param n Normal vector
 * @param tir Whether to calculate the direction of total internal reflection
 * 
 * @return Direction of reflected light
 */
Vec3<float> reflect(const Vec3<float>& wi, const Vec3<float>& n, bool tir = false);

/**
 * @brief Calculate the direction of transmitted light
 * 
 * @param wi Incident direction vector
 * @param n Normal vector
 * @param eta_i Index of refraction of the medium where the incident light originates
 * @param eta_t Index of refraction of the medium where the transmitted light arrives
 * @return Directionmitted direction vector
 */
Vec3<float> transmit(const Vec3<float>& wi, const Vec3<float>& n, float eta_i, float eta_t);

} // namespace spt

#endif