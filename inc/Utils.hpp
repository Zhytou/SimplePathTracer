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
#include <ostream>
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

constexpr float EPS     = 1e-6f;
constexpr float DIS_EPS = 1e-3f;
constexpr float PDF_EPS = 1e-6f;

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
 * @brief Create orthonormal basis(local-to-world transform matrix) based on the given normal vector
 * 
 * @param normal Normal vector
 * @param[out] tangent Tangent vector
 * @param[out] bitangent Bitangent vector
 */
void TBN(const Vec3<float>& normal, Vec3<float>& tangent, Vec3<float>& bitangent);

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

/**
 * @brief Calculate the Fresnel reflection coefficient
 * 
 * @param cos_theta_i Cosine of the angle between the incident light and the normal vector
 * @param eta_i Index of refraction of the medium where the incident light originates
 * @param eta_t Index of refraction of the medium where the transmitted light arrives
 * @return Fresnel reflection coefficient
 */
float fresnel(float cos_theta_i, float eta_i, float eta_t);

Vec3<float> fresnel(float cosThetaI, const Vec3<float>& eta, const Vec3<float>& eta_k);

} // namespace spt

#endif