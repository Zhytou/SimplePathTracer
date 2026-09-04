#include "Material.hpp"
#include "Distribution.hpp"

namespace spt {

Vec3f Diffuse::sample(const Vec3f& wi, Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    CosineDistribution distribution;
    wo = distribution.sample();
    return wi.z > 0 ? m_albedo : Vec3f(0.f);
}

Vec3f Diffuse::eval(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    return m_albedo / PI;
}

float Diffuse::pdf(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv) const {
    CosineDistribution distribution;
    return distribution.pdf(wo);
}

Vec3f Mirror::sample(const Vec3f& wi, Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    wo = {-wi.x, -wi.y, wi.z};
    return wo.z > 0 ? Vec3f(1.f) : Vec3f(0.f);
}

Vec3f Mirror::eval(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    throw std::runtime_error("Mirror::eval Ideal reflection eval is not needed");
}

float Mirror::pdf(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv) const {
    Vec3f h = normalize(wi + wo);

    // only if h is {0, 0, 1} return INF
    return distance(h, Vec3f{0.f, 0.f, 1.f}) < EPS ? INF : 0.f;
}

Vec3f Dielectric::sample(const Vec3f& wi, Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    float eta    = wi.z > 0 ? m_ext_ior / m_int_ior : m_int_ior / m_ext_ior;
    float factor = mode == RADIANCE ? 1.f / (eta * eta) : 1.f;

    float cos_theta_i  = wi.z;
    float sin_theta_i2 = 1.f - cos_theta_i * cos_theta_i;
    float cos_theta_t  = std::sqrt(1 - eta * eta * sin_theta_i2); // NAN for total internal reflection (TIR) case

    float ref       = fresnel(wi.z, m_ext_ior, m_int_ior); // when tir occurs, ref is 1.f
    float prob      = rand(0.f, 1.f);
    bool is_reflect = prob < ref;

    wo     = is_reflect ? Vec3f{-wi.x, -wi.y, wi.z} : Vec3f{-eta * wi.x, -eta * wi.y, cos_theta_i > 0 ? -cos_theta_t : cos_theta_t};
    factor = is_reflect ? 1.f : factor;

    return Vec3f(factor);
}

Vec3f Dielectric::eval(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    throw std::runtime_error("Dielectric::eval Ideal reflection/refraction eval is not needed");
}

float Dielectric::pdf(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv) const {
    bool is_reflect = wi.z * wo.z > 0.f;

    float eta = wi.z > 0 ? m_ext_ior / m_int_ior : m_int_ior / m_ext_ior;
    Vec3f h   = is_reflect ? normalize(wi + wo) : normalize(eta * wi + wo);

    return distance(h, Vec3f{0.f, 0.f, 1.f}) < EPS || distance(h, Vec3f{0.f, 0.f, -1.f}) < EPS ? INF : 0.f;
}

Vec3f MicrofacetConductor::sample(const Vec3f& wi, Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    if (wi.z < 0.f) { return Vec3f(0.f); }

    float roughness = getRoughness(uv);
    GGXDistribution distribution(roughness);

    Vec3f h = distribution.sample(wi);
    wo      = reflect(wi, h);

    Vec3f F  = fresnel(dot(wi, h), m_real_ior, m_imag_ior);
    float G  = distribution.G(wi, wo);
    float G1 = distribution.G1(wi);

    return F * G / G1;
}

Vec3f MicrofacetConductor::eval(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);
    GGXDistribution distribution(roughness);
    Vec3f h = normalize(wi + wo);

    Vec3f F     = fresnel(dot(wi, h), m_real_ior, m_imag_ior);
    float D     = distribution.D(h);
    float G     = distribution.G(wi, wo);
    Vec3f num   = F * D * G;
    float denom = 4.f * std::max(dot(wi, h) * dot(wo, h), EPS);

    return num / denom;
}

float MicrofacetConductor::pdf(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv) const {
    float roughness = getRoughness(uv);
    GGXDistribution distribution(roughness);
    Vec3f h = normalize(wi + wo);

    float jaccobian = 4.f * dot(wo, h);
    return distribution.pdf(wi, h) / std::max(jaccobian, PDF_EPS);
}

Vec3f MicrofacetDielectric::sample(const Vec3f& wi, Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);
    GGXDistribution distribution(roughness);
    Vec3f h = distribution.sample(wi);

    float prob      = rand(0.f, 1.f);
    float ref       = fresnel(dot(wi, h), m_ext_ior, m_int_ior);
    bool is_reflect = prob < ref;

    wo                 = is_reflect ? reflect(wi, h, true) : transmit(wi, h, m_ext_ior, m_int_ior);
    float cos_theta_i  = wi.z;
    float cos_theta_o  = wo.z;
    float cos_theta_h  = h.z;
    float cos_theta_ih = dot(wi, h);
    float cos_theta_oh = dot(wo, h);

    // bsdf * cos_theta_i / pdf
    float G  = distribution.G(wi, wo);
    float G1 = distribution.G1(wi);

    float eta    = cos_theta_i > 0 ? m_ext_ior / m_int_ior : m_int_ior / m_ext_ior;
    float factor = mode == RADIANCE ? eta * eta : 1.f;
    factor       = is_reflect ? 1.f : factor;

    return Vec3f(G / G1 * factor); // visibile ggx sampling
}

Vec3f MicrofacetDielectric::eval(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);
    GGXDistribution distribution(roughness);

    bool is_reflect = wi.z * wo.z > 0.f;
    float eta       = wi.z > 0 ? m_ext_ior / m_int_ior : m_int_ior / m_ext_ior;
    float factor    = mode == RADIANCE ? eta * eta : 1.f;

    Vec3f h = is_reflect ? normalize(wi + wo) : normalize(eta * wi + wo);
    h       = h.z < 0.f ? -h : h; // ensure h is same hemisphere with n(0, 0, 1)

    float cos_theta_i  = wi.z;
    float cos_theta_o  = wo.z;
    float cos_theta_ih = dot(wi, h);
    float cos_theta_oh = dot(wo, h);

    float F     = fresnel(cos_theta_ih, m_ext_ior, m_int_ior);
    float D     = distribution.D(h);
    float G     = distribution.G(wi, wo);
    float num   = is_reflect ? F * D * G : (1 - F) * D * G * std::abs(cos_theta_ih * cos_theta_oh) / std::abs(cos_theta_i * cos_theta_o);
    float denom = is_reflect ? std::max(4.f * std::abs(cos_theta_i) * std::abs(cos_theta_o), PDF_EPS) : std::max(std::pow(eta * cos_theta_ih + cos_theta_oh, 2.f), PDF_EPS);

    factor = is_reflect ? 1.f : factor;
    return Vec3f(num / denom * factor);
}

float MicrofacetDielectric::pdf(const Vec3f& wi, const Vec3f& wo, const Vec2f& uv) const {
    float roughness = getRoughness(uv);
    GGXDistribution distribution(roughness);

    bool is_reflect = wi.z * wo.z > 0.f;
    float eta       = wi.z > 0 ? m_ext_ior / m_int_ior : m_int_ior / m_ext_ior;

    Vec3f h = is_reflect ? normalize(wi + wo) : normalize(eta * wi + wo);
    h       = h.z < 0.f ? -h : h; // ensure h is same hemisphere with n(0, 0, 1)

    float cos_theta_ih = dot(wi, h);
    float cos_theta_oh = dot(wo, h);

    float ref      = fresnel(cos_theta_ih, m_ext_ior, m_int_ior);
    float ratio    = is_reflect ? ref : 1.f - ref;
    float jacobian = is_reflect ? 4.f * std::abs(cos_theta_oh) : std::pow(eta * cos_theta_ih + cos_theta_oh, 2.f) / std::max(std::abs(cos_theta_oh), PDF_EPS);

    return distribution.pdf(wi, h) / std::max(jacobian, PDF_EPS) * ratio;
}

} // namespace spt