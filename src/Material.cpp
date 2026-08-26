#include "Material.hpp"
#include "Distribution.hpp"

namespace spt {

Vec3<float> Diffuse::sample(const Vec3<float>& wi_local, Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    CosineDistribution distribution;
    wo_local = distribution.sample();

    float cos_theta = wo_local.z;
    float pdf       = distribution.pdf(wo_local);
    Vec3f bsdf      = eval(wi_local, wo_local, uv, mode);

    return wi_local.z > 0 ? m_albedo : Vec3<float>(0.f);
    // return wi_local.z > 0 ? bsdf * cos_theta / std::max(pdf, PDF_EPS) : Vec3<float>(0.f);
}

Vec3<float> Diffuse::eval(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    return m_albedo / PI;
}

float Diffuse::pdf(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv) const {
    CosineDistribution distribution;
    return distribution.pdf(wo_local);
}

Vec3<float> Mirror::sample(const Vec3<float>& wi_local, Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    wo_local = {-wi_local.x, -wi_local.y, wi_local.z};
    return wo_local.z > 0 ? Vec3<float>(1.f) : Vec3<float>(0.f);
}

Vec3<float> Mirror::eval(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    throw std::runtime_error("Mirror::eval Ideal reflection eval is not needed");
    // return Vec3<float>(1.f);
}

float Mirror::pdf(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv) const {
    return 0.f;
}

Vec3<float> Dielectric::sample(const Vec3<float>& wi_local, Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    float eta_i        = m_ext_ior;
    float eta_t        = m_int_ior;
    float eta          = wi_local.z > 0 ? eta_i / eta_t : eta_t / eta_i;
    float cos_theta_i  = wi_local.z;
    float sin_theta_i2 = 1.f - cos_theta_i * cos_theta_i;

    float ref  = fresnel(wi_local.z, eta_i, eta_t); // when tir occurs, ref is 1.f
    float prob = rand(0.f, 1.f);

    if (prob < ref) { // reflection or total internal reflection
        wo_local = Vec3<float>{-wi_local.x, -wi_local.y, wi_local.z};
        return Vec3<float>(1.f);
    } else { // transmission
        float cos_theta_t = std::sqrt(1 - eta * eta * sin_theta_i2);
        wo_local          = Vec3<float>{-eta * wi_local.x, -eta * wi_local.y, cos_theta_i > 0 ? -cos_theta_t : cos_theta_t};
        return mode == RADIANCE ? Vec3<float>(1 / (eta * eta)) : Vec3<float>(1.f);
    }
}

Vec3<float> Dielectric::eval(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    throw std::runtime_error("Dielectric::eval Ideal reflection/refraction eval is not needed");
    // float eta_i     = m_ext_ior;
    // float eta_t     = m_int_ior;
    // float eta       = wi_local.z > 0 ? eta_i / eta_t : eta_t / eta_i;
    // bool is_reflect = wi_local.z * wo_local.z > 0.f;

    // return is_reflect ? Vec3<float>(1.f) : Vec3<float>(1 / (eta * eta));
}

float Dielectric::pdf(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv) const {
    return 0.f;
}

Vec3<float> MicrofacetConductor::sample(const Vec3<float>& wi_local, Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);

    GGXDistribution distribution(roughness);
    Vec3<float> h_local = distribution.sample();

    return reflect(wi_local, h_local);
}

Vec3<float> MicrofacetConductor::eval(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);

    GGXDistribution distribution(roughness);
    Vec3<float> h_local = normalize(wi_local + wo_local);

    Vec3<float> F;
    float D         = distribution.D(h_local);
    float G         = distribution.G(wi_local, wo_local);
    Vec3<float> num = F * D * G;
    float denom     = 4.f * std::max(dot(wi_local, h_local) * dot(wo_local, h_local), EPS);

    return num / denom;
}

float MicrofacetConductor::pdf(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv) const {
    float roughness = getRoughness(uv);

    GGXDistribution distribution(roughness);
    Vec3<float> h_local = normalize(wi_local + wo_local);

    float jaccobian = 4.f * dot(wi_local, h_local) * dot(wo_local, h_local);
    float pdf_h     = distribution.pdf(h_local);
    float pdf_w     = pdf_h / std::max(jaccobian, PDF_EPS);

    return pdf_w;
}

Vec3<float> MicrofacetDielectric::sample(const Vec3<float>& wi_local, Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);
    float eta_i     = m_ext_ior;
    float eta_t     = m_int_ior;

    GGXDistribution distribution(roughness);
    Vec3<float> h_local = distribution.sample();

    float prob = rand(0.f, 1.f);
    float ref  = fresnel(dot(wi_local, h_local), eta_i, eta_t);
    wo_local   = prob < ref ? reflect(wi_local, h_local, true) : transmit(wi_local, h_local, eta_i, eta_t);

    float cos_theta = std::abs(wo_local.z);
    Vec3f bsdf      = eval(wi_local, wo_local, uv, mode);
    return bsdf * cos_theta / pdf(wi_local, wo_local, uv);
}

Vec3<float> MicrofacetDielectric::eval(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv, TransportMode mode) const {
    float roughness = getRoughness(uv);
    float eta_i     = m_ext_ior;
    float eta_t     = m_int_ior;
    float eta       = wi_local.z > 0 ? eta_i / eta_t : eta_t / eta_i;
    bool is_reflect = wi_local.z * wo_local.z > 0.f;

    GGXDistribution distribution(roughness);
    Vec3<float> h_local = is_reflect ? normalize(wi_local + wo_local) : normalize(eta * wi_local + wo_local);
    h_local             = h_local.z < 0.f ? -h_local : h_local; // ensure h_local is same hemisphere with n_local(0, 0, 1)

    float cos_theta_i  = wi_local.z;
    float cos_theta_o  = wo_local.z;
    float cos_theta_ih = dot(wi_local, h_local);
    float cos_theta_oh = dot(wo_local, h_local);

    float F     = fresnel(cos_theta_ih, eta_i, eta_t);
    float D     = distribution.D(h_local);
    float G     = distribution.G(wi_local, wo_local);
    float num   = is_reflect ? F * D * G : (1 - F) * D * G * std::abs(cos_theta_ih * cos_theta_oh / (cos_theta_i * cos_theta_o));
    float denom = is_reflect ? std::max(4.f * cos_theta_i * cos_theta_o, PDF_EPS) : std::max(std::pow(eta * cos_theta_ih + cos_theta_oh, 2.f), PDF_EPS);

    return Vec3<float>(num / denom);
}

float MicrofacetDielectric::pdf(const Vec3<float>& wi_local, const Vec3<float>& wo_local, const Vec2<float>& uv) const {
    float roughness = getRoughness(uv);
    float eta_i     = m_ext_ior;
    float eta_t     = m_int_ior;
    float eta       = wi_local.z > 0 ? eta_i / eta_t : eta_t / eta_i;
    bool is_reflect = wi_local.z * wo_local.z > 0.f;

    GGXDistribution distribution(roughness);
    Vec3<float> h_local = is_reflect ? normalize(wi_local + wo_local) : normalize(eta * wi_local + wo_local);
    h_local             = h_local.z < 0.f ? -h_local : h_local; // ensure h_local is same hemisphere with n_local(0, 0, 1)

    float cos_theta_ih = dot(wi_local, h_local);
    float cos_theta_oh = dot(wo_local, h_local);

    float jacobian = is_reflect ? 4.f * std::abs(cos_theta_oh) : std::pow(eta * cos_theta_ih + cos_theta_oh, 2.f) / std::max(std::abs(cos_theta_oh), PDF_EPS);
    float pdf_h    = distribution.pdf(h_local);
    float pdf_w    = pdf_h / std::max(jacobian, PDF_EPS);

    float F = fresnel(cos_theta_ih, eta_i, eta_t);
    pdf_w   = is_reflect ? pdf_w * F : pdf_w * (1 - F);
    return pdf_w;
}

} // namespace spt