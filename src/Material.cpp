#include "Material.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <tiny_obj_loader.h>

namespace spt {

/**
 * @brief Samples a direction for Monte Carlo integration of the material's BSDF.
 *
 * This function performs importance sampling based on the material's surface.
 * For example, it uses cosine-weighted hemisphere sampling for diffuse lobes.
 *
 * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
 * @param N     [in] Surface normal. Must be normalized.
 * @param UV    [in] Texture coordinate.
 * 
 * @return Vec3<float> Sampled outgoing direction (pointing AWAY from the surface).
 */
Vec3<float> Material::scatter(const Vec3<float>& V, const Vec3<float>& N, const Vec2<float>& UV) const {
    Vec3<float> L(0.f, 0.f, 0.f);

    if (isTransmissive()) { // transmit if material supports transmission and probability requirement is met
        float prob = rand(0.0f, 1.0f);
        float ref  = ratio(V, N, UV);

        if (prob > ref) {
            L = transmit(V, N, UV);
        }
    }

    if (L == Vec3<float>(0.f, 0.f, 0.f)) { // reflect if total internal reflection occurs or material only supports reflection
        L = reflect(V, N, UV);
    }

    return L;
}

Vec3<float> Material::reflect(const Vec3<float>& V, const Vec3<float>& N, const Vec2<float>& UV) const {
    Vec3<float> L(0.f, 0.f, 0.f);

    switch (m_type & SURFACE_MASK) {
        // perfect specular reflection
        case MATERIAL_SURFACE_SPECULAR: {
            float NdotV = dot(N, V);
            L           = normalize(N * 2 * NdotV - V);
        } break;
        // glossy reflection
        case MATERIAL_SURFACE_GLOSSY: {
            // GGX importance sampling
            Vec3<float> H = sample(V, N, UV, "GGX");
            float HdotV   = dot(H, V);
            L             = normalize(H * 2 * HdotV - V);
        } break;
        // diffuse reflection
        case MATERIAL_SURFACE_DIFFUSE: {
            // COSINE importance sampling
            L = sample(V, N, UV, "COSINE");
        } break;
    }

    return L;
}

Vec3<float> Material::transmit(const Vec3<float>& V, const Vec3<float>& N, const Vec2<float>& UV) const {
    Vec3<float> L(0.f, 0.f, 0.f);

    // construct transmission direction
    auto constructL = [this](const Vec3<float>& V, const Vec3<float>& N) {
        // cosine incident theta
        float cosThetaI = dot(N, V);

        // create a 'temporary' normal, same hemishpere with V
        Vec3<float> NN = (cosThetaI > 0) ? N : -N;

        // ratio of incident ior to transmitted ior
        // cosThetaI > 0: oustide -> material
        // otherwise: material -> outside
        float eta = (cosThetaI > 0) ? (1.0f / m_ior) : m_ior;

        // square of sine transmitted theta
        cosThetaI        = std::fabs(cosThetaI);
        float sin2ThetaT = eta * eta * (1 - cosThetaI * cosThetaI);
        // return if total internal reflection occurs
        if (sin2ThetaT - 1.f > EPS) {
            return Vec3<float>{0.f};
        }
        // cosine transmitted theta
        float cosThetaT = std::sqrt(1.f - sin2ThetaT);
        // !NOTE: Snell's law (ior₁·sinθ₁ = ior₂·sinθ₂)
        Vec3<float> L = -V * eta + NN * (eta * cosThetaI - cosThetaT);

        return L;
    };

    switch (m_type & SURFACE_MASK) {
        // perfect specular transimission
        case MATERIAL_SURFACE_SPECULAR: {
            // construct L
            L = normalize(constructL(V, N));
            assert(dot(L, N) * dot(V, N) <= 0.f);
        } break;
        case MATERIAL_SURFACE_GLOSSY: {
            // GGX importance sampling
            Vec3<float> H = sample(V, N, UV, "GGX");
            // construct L
            L = normalize(constructL(V, H));
        } break;
        default: {
            L = Vec3<float>(0.f, 0.f, 0.f);
        }
    }

    return L;
}

float Material::pdf(const Vec3<float>& V, const Vec3<float>& N, const Vec3<float>& L, const Vec2<float>& UV) const {
    float pdf = 0.f;

    switch (m_type & SURFACE_MASK) {
        // perfect specular reflection or transimission
        case MATERIAL_SURFACE_SPECULAR: {
            pdf = INFINITY; // delta distribution
        } break;
        // glossy reflection or transimission
        case MATERIAL_SURFACE_GLOSSY: {
            Vec3<float> H = normalize(V + L);
            float HdotV   = std::max(dot(H, V), 0.f);
            float HdotN   = std::max(dot(H, N), 0.f);
            float D       = GGX_D(HdotN, UV);
            pdf           = D * HdotN / (4 * HdotV + EPS); // avoid division by zero

            // TODO: add pdf calculation for glossy transimission
            // Vec3<float> H = normalize(V * eta + L);
            // float HdotV = dot(H, V);
            // float HdotL = dot(H, L);
            // float HdotN = dot(H, N);
            // float eta = (HdotV > 0) ? (1.0f / ior) : ior;
            // float D = GGX_D(HdotN, roughness);
            // float num = D * eta * eta * std::fabs(HdotL);
            // float denom = std::max(std::pow(eta * HdotV + HdotL, 2.f), EPS);
            // PDF = num / denom;
        } break;
        // diffuse reflection
        case MATERIAL_SURFACE_DIFFUSE: {
            float NdotL = std::max(dot(N, L), 0.f);
            pdf         = NdotL / PI;
        } break;
        default: {
            pdf = 0.f;
        }
    }

    return pdf;
}

float Material::ratio(const Vec3<float>& V, const Vec3<float>& N, const Vec2<float>& UV) const {
    float NdotV    = dot(N, V);
    Vec3<float> F0 = Fresnel_Zero(UV);
    Vec3<float> F  = Fresnel_Schlick(NdotV, F0);

    return std::clamp(0.2126f * F.x + 0.7152f * F.y + 0.0722f * F.z, 0.0f, 1.0f);
}

Vec3<float> Material::sample(const Vec3<float>& V, const Vec3<float>& N, const Vec2<float>& UV, const std::string& mode) const {
    float a = rand(0.0f, 1.f);
    float b = rand(0.0f, 1.f);

    // local sampling direction
    Vec3<float> localDir(0.f, 0.f, 0.f);

    if (mode == "GGX") {
        float roughness = getRoughness(UV);
        float alpha     = roughness * roughness;
        float alpha2    = alpha * alpha;

        float cosTheta = std::sqrt((1.f - a) / (1.f + (alpha2 - 1.f) * a));
        float sinTheta = std::sqrt(1.f - cosTheta * cosTheta);
        float Phi      = 2 * PI * b;

        localDir = {cosf(Phi) * sinTheta, sinf(Phi) * sinTheta, cosTheta};
    } else if (mode == "COSINE") {
        float cosTheta = std::sqrt(a);
        float sinTheta = std::sqrt(1.f - cosTheta * cosTheta);
        float Phi      = 2 * PI * b;

        localDir = {cosf(Phi) * sinTheta, sinf(Phi) * sinTheta, cosTheta};
    } else {
        return localDir; // return default
    }

    // orthogonal basis
    Vec3<float> UP = (std::fabs(N.z) < 0.999f) ? Vec3<float>(0.f, 0.f, 1.f) : Vec3<float>(0.f, 1.f, 0.f);
    Vec3<float> E1 = normalize(cross(UP, N));
    Vec3<float> E2 = normalize(cross(N, E1));
    // convert to world coordinates
    Vec3<float> worldDir = E1 * localDir.x + E2 * localDir.y + N * localDir.z;

    return worldDir;
}

/**
 * @brief Evaluates the BSDF value for given direction and point.
 * 
 * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
 * @param N     [in] Surface normal. Must be normalized.
 * @param L     [in] Incident light direction (pointing AWAY from the surface). Must be normalized.
 * @param UV    [in] Texture coordinates.
 * 
 * @return Vec3<float> The computed BSDF value.
 */
Vec3<float> Material::bsdf(const Vec3<float>& V, const Vec3<float>& N, const Vec3<float>& L, const Vec2<float>& UV) const {
    Vec3<float> bsdf(0.f, 0.f, 0.f);

    float NdotV = dot(N, V);
    float NdotL = dot(N, L);

    if (isTransmissive() && NdotV * NdotL < 0) { // transimission
        // create a 'temporary' normal, same hemishpere with V
        Vec3<float> NN = (NdotV > 0) ? N : -N;

        // ratio of ior
        float eta = (NdotV > 0) ? 1.0f / m_ior : m_ior;

        // half vector
        Vec3<float> H = normalize(V * eta + L);

        // make sure H is in same hemisphere with NN
        H = (dot(NN, H) > 0) ? H : -H;

        // !NOTE: make sure V and N in the same hemisphere, while L in the opposite one
        // !NOTE: also make sure transmission happens
        if (dot(V, NN) > 0 && dot(L, NN) < 0 && dot(V, H) > 0 && dot(L, H) < 0) {
            // btdf
            bsdf = btdf(V, NN, L, H, UV, eta);
        }

    } else if (isTransmissive() && NdotV < 0 && NdotL < 0) { // total internal reflection
        // ratio of ior
        float eta = m_ior;
        // cosine of incident angle
        float cosThetaI = std::fabs(NdotV);
        // square of sine of transmission angle
        float sin2ThetaT = eta * eta * (1.0f - cosThetaI * cosThetaI);

        // half vector
        Vec3<float> H = normalize(V + L);

        // !NOTE: make sure TIR occurs, which means sin2ThetaT is invalid(no sin should be larger than 1)
        if (sin2ThetaT - 1 > EPS) {
            // brdf_tir
            bsdf = brdf_tir(V, N, L, H, UV);
        }
    } else { // common reflection
        // half vector
        Vec3<float> H = normalize(V + L);

        // !NOTE: make sure V, L, N, H in the same hemisphere
        if (NdotV > 0 && NdotL > 0) {
            // brdf
            bsdf = brdf(V, N, L, H, UV);
        }
    }

    return bsdf;
}

Vec3<float> Material::brdf(const Vec3<float>& V, const Vec3<float>& N, const Vec3<float>& L, const Vec3<float>& H, const Vec2<float>& UV) const {
    // material properties
    Vec3<float> color = getAlbedo(UV);
    float metallic    = getMetallic(UV);

    // cosine constants
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);
    float VdotH = dot(V, H);

    // all the directions must be in same hemisphere
    if (NdotL < 0 || NdotV < 0 || NdotH < 0) {
        throw std::runtime_error("Material::brdf: V, N and L directions must be in same hemisphere!");
    }

    float D        = GGX_D(NdotH, UV);
    float G        = Smith_G(NdotV, NdotL, UV);
    Vec3<float> F0 = Fresnel_Zero(UV);
    Vec3<float> F  = Fresnel_Schlick(VdotH, F0);

    switch (m_type & SURFACE_MASK) {
        case MATERIAL_SURFACE_DIFFUSE: {
            return (1 - metallic) * color / PI;
        }
        case MATERIAL_SURFACE_GLOSSY: {
            float denom = std::max(4.0f * NdotV * NdotL, EPS);
            return (1 - metallic) * color / PI + F * D * G / denom;
        }
        case MATERIAL_SURFACE_SPECULAR: {
            // H should be same as N for perfect reflection
            // assert(distance(N, H) < EPS);
            // !NOTE: F already encodes the color and reflectance
            // !      so for perfect specular reflection, return F directly—no diffuse or albedo scaling needed.
            return F;
        }
        default: {
            return Vec3<float>(0.f);
        }
    }
}

Vec3<float> Material::brdf_tir(const Vec3<float>& V, const Vec3<float>& N, const Vec3<float>& L, const Vec3<float>& H, const Vec2<float>& UV) const {
    switch (m_type & SURFACE_MASK) {
        case MATERIAL_SURFACE_SPECULAR: {
            return Vec3<float>(1.f);
        }
        case MATERIAL_SURFACE_GLOSSY: {
            float NdotL = dot(N, L);
            float NdotV = dot(N, V);
            float NdotH = dot(N, H);

            float D         = GGX_D(NdotH, UV);
            float G         = Smith_G(NdotV, NdotL, UV);
            Vec3<float> F   = Vec3<float>(1.0f);
            Vec3<float> num = F * D * G;
            float denom     = std::max(4.0f * NdotV * NdotL, EPS);

            return num / denom;
        }
        case MATERIAL_SURFACE_DIFFUSE: {
            return Vec3<float>(0.f);
        }
        default: {
            return Vec3<float>(0.f);
        }
    }
}

Vec3<float> Material::btdf(const Vec3<float>& V, const Vec3<float>& N, const Vec3<float>& L, const Vec3<float>& H, const Vec2<float>& UV, float eta) const {
    // cosine constants
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);
    float VdotH = dot(V, H);

    // make sure V, H, N are in the same hemisphere, while L in the opposite one
    assert(NdotL <= 0 && NdotV >= 0 && NdotH >= 0);

    // return if total internal reflection occurs
    if ((1 - VdotH * VdotH) * eta * eta > 1.f) {
        return Vec3<float>(0.f, 0.f, 0.f);
    }

    Vec3<float> F0 = Fresnel_Zero(UV);
    Vec3<float> F  = Fresnel_Schlick(VdotH, F0);
    float D        = GGX_D(NdotH, UV);
    float G        = Smith_G(NdotV, std::fabs(NdotL), UV);
    Vec3<float> NF = Vec3<float>(1.f) - F;

    switch (m_type & SURFACE_MASK) {
        case MATERIAL_SURFACE_SPECULAR: {
            // H should be same as N for perfect transmission
            // assert(distance(N, H) < EPS);
            return NF / (eta * eta);
        }
        case MATERIAL_SURFACE_GLOSSY: {
            float denom     = std::max(std::pow(eta * VdotH + LdotH, 2.f) * std::fabs(NdotV * NdotL), EPS);
            Vec3<float> num = NF * D * G * eta * eta * std::fabs(VdotH * LdotH);
            return num / denom;
        }
        case MATERIAL_SURFACE_DIFFUSE: {
            return Vec3<float>(0.f);
        }
        default: {
            return Vec3<float>(0.f);
        }
    }
}

Vec3<float> Material::Fresnel_Zero(const Vec2<float>& UV) const {
    Vec3<float> F0(0.04f); // default reflectance at normal incidence

    Vec3<float> albedo = getAlbedo(UV);
    float metallic     = getMetallic(UV);
    float ior          = getIOR();

    if (isTransmissive()) {
        F0 = pow(Vec3<float>(ior - 1.f) / Vec3<float>(ior + 1.f), 2.f);
    }
    F0 = F0 * (1 - metallic) + albedo * metallic; // mix based on metallic. For metals, baseColor = F0 (reflectance at normal incidence); For non-metals, F0 ≈ 0.04.

    return F0;
}

Vec3<float> Material::Fresnel_Schlick(float cosTheta, const Vec3<float>& F0) const {
    cosTheta = std::fabs(cosTheta); // make sure cosTheta is positive for Schlick's approximation
    return F0 + (Vec3<float>(1.f, 1.f, 1.f) - F0) * std::pow(1.0f - cosTheta, 5.0f);
}

float Material::GGX_D(float NdotH, const Vec2<float>& UV) const {
    float roughness = getRoughness(UV);
    float alpha     = roughness * roughness;
    float alpha2    = alpha * alpha;
    float NdotH2    = NdotH * NdotH;

    float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
    denom       = PI * denom * denom;
    return alpha2 / denom;
}

float Material::Smith_G(float NdotV, float NdotL, const Vec2<float>& UV) const {
    float roughness         = getRoughness(UV);
    auto GeometrySchlickGGX = [roughness](float cosTheta) { // SIGGRAPH 2013：UE4
        float r = (roughness + 1.0f);
        float k = (r * r) / 8.0f;

        float num   = cosTheta;
        float denom = cosTheta * (1.0f - k) + k;

        return num / denom;
    };

    float ggx1 = GeometrySchlickGGX(NdotV);
    float ggx2 = GeometrySchlickGGX(NdotL);

    return ggx1 * ggx2;
}

} // namespace spt
