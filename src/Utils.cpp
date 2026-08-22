#include "Utils.hpp"

namespace spt {

void TBN(const Vec3f& normal, Vec3f& tangent, Vec3f& bitangent) {
    // Duff et al. 2017 Building an Orthonormal Basis, Revisited
    float sign = std::copysign(1.0f, normal.z);
    float a    = -1.0f / (sign + normal.z);
    float c    = normal.x * normal.y * a;
    tangent    = Vec3<float>(1.0f + sign * normal.x * normal.x * a, sign * c, -sign * normal.x);
    bitangent  = Vec3<float>(c, sign + normal.y * normal.y * a, -normal.y);
}

Vec3f toLocal(const Vec3f& point, const Vec3f& tangent, const Vec3f& bitangent, const Vec3f& normal) {
    return {dot(point, tangent), dot(point, bitangent), dot(point, normal)};
}

Vec3f toWorld(const Vec3f& point, const Vec3f& tangent, const Vec3f& bitangent, const Vec3f& normal) {
    return point.x * tangent + point.y * bitangent + point.z * normal;
}

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

Vec3f reflect(const Vec3f& wi, const Vec3f& n, bool tir) {
    float cos_theta_i = dot(wi, n);
    if (cos_theta_i < 0 && !tir) {
        throw std::runtime_error("reflect: Cosine of incident angle is negative and total internal reflection is not allowed.");
    }
    Vec3<float> nn = cos_theta_i > 0 ? n : -n;
    cos_theta_i    = std::abs(cos_theta_i);
    Vec3<float> wo = 2 * nn * cos_theta_i - wi;
    return wo;
}

Vec3f transmit(const Vec3f& wi, const Vec3f& n, float eta_i, float eta_t) {
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
    if (cos_theta_i < 0) {
        cos_theta_i = -cos_theta_i;
        std::swap(eta_i, eta_t);
    }
    float eta = eta_i / eta_t;

    float sin_theta_t2 = eta * eta * (1 - cos_theta_i * cos_theta_i);
    float cos_theta_t  = std::sqrt(1.0f - sin_theta_t2);
    if (sin_theta_t2 > 1.0f) { // total internal reflection
        return 1.0f;
    }

    float Rs = (eta_i * cos_theta_i - eta_t * cos_theta_t) / (eta_i * cos_theta_i + eta_t * cos_theta_t);
    float Rp = (eta_t * cos_theta_i - eta_i * cos_theta_t) / (eta_t * cos_theta_i + eta_i * cos_theta_t);

    return (Rs * Rs + Rp * Rp) * 0.5f;
}

Vec3f fresnel(float cos_theta_i, const Vec3f& eta, const Vec3f& k) {
    cos_theta_i = std::clamp(cos_theta_i, -1.0f, 1.0f);

    float cos_theta_i2 = cos_theta_i * cos_theta_i;
    float sin_theta_i2 = 1. - cos_theta_i2;
    Vec3<float> eta2   = eta * eta;
    Vec3<float> etak2  = k * k;

    Vec3<float> t0       = eta2 - etak2 - Vec3<float>(sin_theta_i2);
    Vec3<float> a2plusb2 = pow(t0 * t0 + 4 * eta2 * etak2, 0.5f);
    Vec3<float> t1       = a2plusb2 + Vec3<float>(cos_theta_i2);
    Vec3<float> a        = pow(0.5f * (a2plusb2 + t0), 0.5f);
    Vec3<float> t2       = 2.f * cos_theta_i * a;
    Vec3<float> Rs       = (t1 - t2) / (t1 + t2);

    Vec3<float> t3 = cos_theta_i2 * a2plusb2 + Vec3<float>(sin_theta_i2 * sin_theta_i2);
    Vec3<float> t4 = t2 * sin_theta_i2;
    Vec3<float> Rp = Rs * (t3 - t4) / (t3 + t4);

    return (Rp + Rs) * 0.5f;
}

} // namespace spt
