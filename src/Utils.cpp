#include "Utils.hpp"

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
        throw std::runtime_error("reflect: Cosine of incident angle is negative and total internal reflection is not allowed.");
    }
    Vec3<float> nn = cos_theta_i > 0 ? n : -n;
    cos_theta_i    = std::abs(cos_theta_i);
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

    // aboslute value of cosine incident theta
    cos_theta_i = std::abs(cos_theta_i);
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

float fresnel(float cos_theta_i, float eta_i, float eta_t) {
    if (eta_i == eta_t) {
        return 0.0f;
    }
    float eta = cos_theta_i > 0 ? eta_i / eta_t : eta_t / eta_i;

    cos_theta_i        = std::abs(cos_theta_i);
    float sin_theta_t2 = eta * eta * (1 - cos_theta_i * cos_theta_i);
    float cos_theta_t  = std::sqrt(1.0f - sin_theta_t2);
    if (sin_theta_t2 > 1.0f) { // total internal reflection
        return 1.0f;
    }

    float Rs = (eta_i * cos_theta_i - eta_t * cos_theta_t) / (eta_i * cos_theta_i + eta_t * cos_theta_t);
    float Rp = (eta_t * cos_theta_i - eta_i * cos_theta_t) / (eta_t * cos_theta_i + eta_i * cos_theta_t);

    return (Rs * Rs + Rp * Rp) / 2.0f;
}

Vec3<float> fresnel(float cosThetaI, const Vec3<float>& eta, const Vec3<float>& eta_k) {
    return Vec3<float>(0.0f);

    // cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);

    // float cosThetaI2 = cosThetaI * cosThetaI;
    // float sinThetaI2 = 1. - cosThetaI2;
    // Color3f eta2     = eta * eta;
    // Color3f etak2    = k * k;

    // Color3f t0       = eta2 - etak2 - sinThetaI2;
    // Color3f a2plusb2 = sqrt(t0 * t0 + 4 * eta2 * etak2);
    // Color3f t1       = a2plusb2 + cosThetaI2;
    // Color3f a        = sqrt(0.5f * (a2plusb2 + t0));
    // Color3f t2       = (float)2 * cosThetaI * a;
    // Color3f Rs       = (t1 - t2) / (t1 + t2);

    // Color3f t3 = cosThetaI2 * a2plusb2 + sinThetaI2 * sinThetaI2;
    // Color3f t4 = t2 * sinThetaI2;
    // Color3f Rp = Rs * (t3 - t4) / (t3 + t4);

    // return 0.5 * (Rp + Rs);
}

} // namespace spt
