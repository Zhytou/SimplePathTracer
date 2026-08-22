#ifndef SPT_UTILS_HPP
#define SPT_UTILS_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <random>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "Mat.hpp"
#include "Vec.hpp"

namespace spt {

constexpr float EPS     = 1e-12f;
constexpr float DIS_EPS = 1e-3f;
constexpr float PDF_EPS = 1e-9f;

constexpr float PI = 3.14159265358979323846f;

template <typename T>
T rand(T min, T max) {
    static_assert(std::is_arithmetic<T>::value, "T must be numeric type");

    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());

    if constexpr (std::is_integral<T>::value) {
        std::uniform_int_distribution<T> dis(min, max);
        return dis(gen);
    } else {
        std::uniform_real_distribution<T> dis(min, max);
        return dis(gen);
    }
}

constexpr float radians(float deg) {
    return deg * PI / 180.f;
}

constexpr float degrees(float rad) {
    return rad * 180.f / PI;
}

/**
 * @brief  Convert the area PDF to the solid angle PDF
 * @param pdf_a Area PDF
 * @param dis Distance between the shading point and the light source
 * @param cos_theta Cosine of the angle between light surface normal and outgoing light direction
 * @return Corresponding solid-angle PDF defined over direction space
 */
constexpr float a2w(float pdf_a, float dis, float cos_theta) { return pdf_a * dis * dis / std::max(std::abs(cos_theta), PDF_EPS); }

/**
 * @brief  Convert the solid angle PDF to the area PDF
 * @param pdf_w Solid-angle probability density defined over direction space
 * @param dis Distance between shading point and sampled light point
 * @param cos_theta Cosine of the angle between light surface normal and outgoing light direction
 * @return Corresponding area PDF defined over light surface area
 */
constexpr float w2a(float pdf_w, float dis, float cos_theta) { return pdf_w * std::abs(cos_theta) / std::max(dis * dis, PDF_EPS); }

/**
 * @brief Create orthonormal basis(local-to-world transform matrix) based on the given normal vector
 * 
 * @param normal Normal vector
 * @param[out] tangent Tangent vector
 * @param[out] bitangent Bitangent vector
 */
void TBN(const Vec3f& normal, Vec3f& tangent, Vec3f& bitangent);

/**
 * @brief Convert a point from world space to local space
 * 
 * @param point World-space point
 * @param tangent Tangent vector
 * @param bitangent Bitangent vector
 * @param normal Normal vector
 * @return Local-space point
 */
Vec3f toLocal(const Vec3f& point, const Vec3f& tangent, const Vec3f& bitangent, const Vec3f& normal);

/**
 * @brief Convert a point from local space to world space
 * 
 * @param point Local-space point
 * @param tangent Tangent vector
 * @param bitangent Bitangent vector
 * @param normal Normal vector
 * @return World-space point
 */
Vec3f toWorld(const Vec3f& point, const Vec3f& tangent, const Vec3f& bitangent, const Vec3f& normal);

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
 * @param tir Indicator of total internal reflection allowed
 * 
 * @return Direction of reflected light
 */
Vec3f reflect(const Vec3f& wi, const Vec3f& n, bool tir = false);

/**
 * @brief Calculate the direction of transmitted light
 * 
 * @param wi Incident direction vector
 * @param n Normal vector
 * @param eta_i Index of refraction of the medium where the incident light originates
 * @param eta_t Index of refraction of the medium where the transmitted light arrives
 * @return Directionmitted direction vector
 */
Vec3f transmit(const Vec3f& wi, const Vec3f& n, float eta_i, float eta_t);

/**
 * @brief Calculate the Fresnel reflection coefficient
 * 
 * @param cos_theta_i Cosine of the angle between the incident light and the normal vector
 * @param eta_i Index of refraction of the medium where the incident light originates
 * @param eta_t Index of refraction of the medium where the transmitted light arrives
 * @return Fresnel reflection coefficient
 */
float fresnel(float cos_theta_i, float eta_i, float eta_t);

Vec3f fresnel(float cosThetaI, const Vec3f& eta, const Vec3f& eta_k);

} // namespace spt

#endif