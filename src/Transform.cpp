#include "Transform.hpp"

namespace spt {

Mat4x4f translate(const Vec3f& t) {
    Mat4x4f mat = Mat4x4f::eye();
    mat(0, 3)   = t[0];
    mat(1, 3)   = t[1];
    mat(2, 3)   = t[2];
    return mat;
}

Mat4x4f scale(const Vec3f& s) {
    Mat4x4f mat = Mat4x4f::eye();
    mat(0, 0)   = s[0];
    mat(1, 1)   = s[1];
    mat(2, 2)   = s[2];
    return mat;
}

Mat4x4f rotate(int axis, float rad) {
    Mat4x4f mat = Mat4x4f::eye();
    switch (axis) {
        case 0: {
            float c = std::cos(rad), s = std::sin(rad);
            mat(1, 1) = c;
            mat(1, 2) = -s;
            mat(2, 1) = s;
            mat(2, 2) = c;
        } break;
        case 1: {
            float c   = std::cos(rad);
            float s   = std::sin(rad);
            mat(0, 0) = c;
            mat(0, 2) = s;
            mat(2, 0) = -s;
            mat(2, 2) = c;
        } break;
        case 2: {
            float c   = std::cos(rad);
            float s   = std::sin(rad);
            mat(0, 0) = c;
            mat(0, 1) = -s;
            mat(1, 0) = s;
            mat(1, 1) = c;
        } break;
        default: {
            throw std::invalid_argument("axis must be 0, 1, or 2");
        }
    }

    return mat;
}

Mat4x4f rotate(const Vec3f& d) {
    Mat4x4f mat = Mat4x4f::eye();
    for (int i = 0; i < 3; ++i) {
        mat = rotate(i, radians(d[i])) * mat;
    }
    return mat;
}

Vec3<float> reflect(const Vec3<float>& wi, const Vec3<float>& n, bool tir) {
    float cos_theta_i = dot(wi, n);
    if (cos_theta_i < 0 && !tir) {
        throw std::runtime_error("reflect: Cosine of incident angle is negative.");
    }
    Vec3<float> nn = cos_theta_i > 0 ? n : -n;
    Vec3<float> wo = 2 * nn * cos_theta_i - wi;
    return wo;
}

Vec3<float> transmit(const Vec3<float>& wi, const Vec3<float>& n, float eta_i, float eta_t) {
    // cosine incident theta
    float cos_theta_i = dot(wi, n);
    // relative IOR
    float eta = cos_theta_i > 0 ? eta_i / eta_t : eta_t / eta_i;
    // temporary normal vector same hemisphere with wi
    Vec3<float> nn = cos_theta_i > 0 ? n : -n;
    // square of sine transmitted theta
    float sin_theta_t2 = eta * eta * (1 - cos_theta_i * cos_theta_i);
    // return if total internal reflection occurs
    if (sin_theta_t2 > 1.f) {
        throw std::runtime_error("Material::transmit: Total internal reflection occurs.");
    }
    // cosine transmitted theta
    float cos_theta_t = std::sqrt(1.f - sin_theta_t2);

    // snell's law (ior₁·sinθ₁ = ior₂·sinθ₂)
    Vec3<float> wo = -wi * eta + nn * (eta * cos_theta_i - cos_theta_t);
    return wo;
}

} // namespace spt
