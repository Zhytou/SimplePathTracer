#include "Material.hpp"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <tiny_obj_loader.h>

namespace spt
{
    uint Material::scatMask = 0b0000011;

    uint Material::surfMask = 0b0011100;

    uint Material::illuMask = 0b1100000;

    Material::Material(const tinyobj::material_t& mtl, const std::string& dir, uint illuType) {
        // name
        name = mtl.name;

        // emissive
        emissive = false;

        // diffuse
        diffuse = mtl.diffuse;

        // metallic
        metallic = mtl.metallic;
        
        // roughness
        roughness = mtl.roughness;

        // specular
        specular = mtl.specular;

        // shineness
        shininess = mtl.shininess;

        //! NOTE: BRDF sampling uses roughness for GGX; convert shininess to roughness for Phong model.
        if (roughness == 0 && illuType != BSDF_MICROFACET) {
            roughness = ::powf(10.f / (shininess + 10.f), 1 / 8.f);
        }
        
        // ior
        ior = mtl.ior;

        // Kd Texture
        if (!mtl.diffuse_texname.empty()) {
            auto texName = dir+mtl.diffuse_texname;
            albedo = Texture::getInstance(texName);
        } else {
            albedo = nullptr;
        }

        // type
        type = illuType;
        // scatter type
        if (ior > 1.f) {
            type = type | (BSDF_REFLECTION | BSDF_TRANSIMISSION);
        } else {
            type = type | BSDF_REFLECTION;
        }
        // surface type
        if (roughness < 0.01f || (type & BSDF_TRANSIMISSION)) {
            // !NOTE: if surface is transparent, must be specular
            // TODO: add GLOSSY trasmission
            type = type | BSDF_SPECULAR;
        } else if (roughness > 0.7f) {
            type = type | BSDF_DIFFUSE;
        } else {
            type = type | BSDF_GLOSSY;
        }
    }

    bool Material::isDelta() const {
        return (type & surfMask) == BSDF_SPECULAR;
    }

    bool Material::isEmissive() const { 
        return emissive;
    }

    void Material::setEmission(Vec3<float> e) { 
        emissive = true;
        emission = e;
    }

    uint Material::getType() const {
        return type;
    }

    std::string Material::getName() const {
        return name;
    }

    Vec3<float> Material::getEmission() const {
        if (emissive) {
            return emission;
        } else {
            return Vec3(0.f, 0.f, 0.f);
        }
    }

    Vec3<float> Material::getBaseColor(Vec2<float> uv) const {
        if (albedo == nullptr) {
            return diffuse;
        } else {
            return albedo->getColorAt(uv);
        }
    }

    Vec3<float> Material::getFresnel0(const Vec3<float>& baseColor) const {
        Vec3<float> F0(0.04f, 0.04f, 0.04f);

        if (type & BSDF_TRANSIMISSION) { // dielectric (supports both reflection and transmission)
            F0 = Vec3<float>(ior, ior, ior);
            F0 = pow((F0 - Vec3(1.f, 1.f, 1.f)) / (F0 + Vec3(1.f, 1.f, 1.f)), 2.f);
        } else { // opaque (supports only reflection)
            // for metals, baseColor = F0 (reflectance at normal incidence);
            // for non-metals, F0 ≈ 0.04. Mix based on metallic.
            F0 = F0 * (1 - metallic) + baseColor * metallic;
        }
        
        return F0;
    }

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
    Vec3<float> Material::scatter(const Vec3<float> &V, const Vec3<float> &N, const Vec2<float>& UV) const {
        Vec3<float> L(0.f, 0.f, 0.f);
        float prob = rand(1.f);
        Vec3<float> color = getBaseColor(UV);
        float F0 = getFresnel0(color).max();
        
        if ((type & BSDF_TRANSIMISSION) && prob > F0) {
            L = transmit(V, N);
        }
        
        // reflect if total internal reflection occurs or material only supports reflection
        if (L == Vec3<float>(0.f, 0.f, 0.f)) {
            L = reflect(V, N, UV);
        }

        return L;
    }

    Vec3<float> Material::reflect(const Vec3<float> &V, const Vec3<float> &N, const Vec2<float>& UV) const {
        Vec3<float> L(0.f, 0.f, 0.f);

        // bsdf surface type
        uint surfType = type & surfMask;

        switch (surfType) {
            // perfect specular reflection
            case BSDF_SPECULAR: {
                float NdotV = dot(N, V);
                L = normalize(N * 2 * NdotV - V);
            }
            break;
            // glossy reflection
            case BSDF_GLOSSY: {
                // GGX and COSINE combined importance sampling
                Vec3<float> color = getBaseColor(UV);
                float F0 = getFresnel0(color).max();

                if (rand(1.f) > F0) {
                    L = sample(V, N, "COSINE");
                } else {
                    Vec3<float> H = sample(V, N, "GGX");
                    float HdotV = dot(H, V);
                    L = normalize(H * 2 * HdotV - V);
                }
            }
            break;
            // diffuse reflection
            case BSDF_DIFFUSE: {
                // COSINE importance sampling
                L = sample(V, N, "COSINE");
            }
            break;
        }

        return L;
    }

    Vec3<float> Material::transmit(const Vec3<float> &V, const Vec3<float> &N) const {
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
            float eta = (cosThetaI > 0) ? (1.0f / ior) : ior; 

            // square of sine transmitted theta
            float sin2ThetaT = eta * eta * (1 - cosThetaI * cosThetaI);
            // return if total internal reflection occurs
            if (sin2ThetaT > 1) {
                return Vec3<float>{0.f, 0.f, 0.f};
            }
            // cosine transmitted theta
            float cosThetaT = sqrtf(1 - sin2ThetaT);

            // !NOTE: Snell's law (ior₁·sinθ₁ = ior₂·sinθ₂)
            Vec3<float> L = - V * eta + NN * (eta * cosThetaI - cosThetaT);
            return L;
        };

        // bsdf surface type
        uint surfType = type & surfMask;

        switch (surfType) {
            // perfect specular trasimission
            case BSDF_SPECULAR: {
                // construct L
                L = constructL(V, N);
            }
            break;
            case BSDF_GLOSSY: {
                // GGX importance sampling
                Vec3<float> H = sample(V, N, "GGX");
                // construct L
                L = normalize(constructL(V, H));
            }
            break;
        }

        return L;
    }

    float Material::pdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec2<float>& UV) const {
        float PDF = 0.f;

        // bsdf surface type
        uint surfType = type & surfMask;

        switch (surfType) {
            // perfect specular reflection or transimission
            case BSDF_SPECULAR: {
                PDF = 0.f;
            }
            break;
            // glossy reflection or transimission
            case BSDF_GLOSSY: {
                // pdf of GGX and COSINE combined importance sampling
                Vec3<float> color = getBaseColor(UV);
                float F0 = getFresnel0(color).max();

                Vec3<float> H = normalize(V + L);
                float HdotV = dot(H, V);
                float HdotN = dot(H, N);
                float NdotL = dot(N, L);
                float D = GGX_D(HdotN, roughness);
                float denom = std::max(4 * HdotV, EPSILON);
                PDF = F0 * D * HdotN / denom + (1 - F0) *  NdotL / PI; 

                // TODO: add pdf calculation for glossy transimission
                // Vec3<float> H = normalize(V * eta + L);
                // float HdotV = dot(H, V);
                // float HdotL = dot(H, L);
                // float HdotN = dot(H, N);
                // float eta = (HdotV > 0) ? (1.0f / ior) : ior;
                // float D = GGX_D(HdotN, roughness);
                // float nom = D * eta * eta * ::fabsf(HdotL);
                // float denom = std::max(::powf(eta * HdotV + HdotL, 2.f), EPSILON);
                // PDF = nom / denom; 
            }
            break;
            // diffuse reflection
            case BSDF_DIFFUSE: {
                float NdotL = dot(N, L);
                PDF = NdotL / PI;            
            }
            break;
        }
        
        return PDF;
    }

    Vec3<float> Material::sample(const Vec3<float> &V, const Vec3<float> &N, const std::string& mode) const {
        float a = rand(1.f), b = rand(1.f);

        // local sampling direction
        Vec3<float> localDir(0.f, 0.f, 0.f);

        if (mode == "GGX") {
            float alpha = roughness * roughness;
            float alpha2 = alpha * alpha;

            float cosTheta = sqrtf((1.f - a) / (1.f + (alpha2 - 1.f) * a));
            float sinTheta = sqrtf(1.f - cosTheta * cosTheta);
            float Phi = 2 * PI * b;

            localDir = {cosf(Phi) * sinTheta, sinf(Phi) * sinTheta, cosTheta};
        } else if (mode == "COSINE") {
            float cosTheta = sqrtf(a);
            float sinTheta = sqrtf(1.f - cosTheta * cosTheta);
            float Phi =  2 * PI * b;

            localDir = {cosf(Phi) * sinTheta, sinf(Phi) * sinTheta, cosTheta};
        } else {
            return localDir; // return default
        }

        // orthogonal basis
        Vec3<float> E1 = normalize(cross(V, N));
        Vec3<float> E2 = normalize(cross(E1, N));
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
    Vec3<float> Material::bsdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec2<float>& UV) const {
        Vec3<float> bsdf(0.f, 0.f, 0.f);

        if ((type & BSDF_TRANSIMISSION) && dot(V, N) * dot(L, N) < 0.f) { // transimission and view light direction in different hemisphere
            // create a 'temporary' normal, same hemishpere with V
            Vec3<float> NN = (dot(N, V) > 0) ? N : -N;

            // ratio of ior
            float eta = (dot(N, V) > 0) ? 1.0f / ior : ior;

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
        } else {// reflection 
            // half vector
            Vec3<float> H = normalize(V + L);

            // !NOTE: make sure V, L, N, H in the same hemisphere
            if (dot(V, N) > 0 && dot(L, N) > 0) {
                // brdf
                bsdf = brdf(V, N, L, H, UV);
            }
        }
        
        return bsdf;
    }

    Vec3<float> Material::brdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const {
        // brdf surface type
        uint surfType = type & surfMask;

        // bsdf illumination model
        uint illuType = type & illuMask;

        // material color
        Vec3<float> baseColor = getBaseColor(UV);
        // Fresnel reflectance at normal incidence
        Vec3<float> F0 = getFresnel0(baseColor);
    
        // cosine constants
        float NdotL = dot(N, L);
        float NdotV = dot(N, V);
        float NdotH = dot(N, H);
        float LdotH = dot(L, H);
        float VdotH = dot(V, H);
        
        // all the directions must be in same hemisphere
        assert(NdotL >= 0 && NdotV >= 0 && NdotH >= 0);
            
        float D = GGX_D(NdotH, roughness);
        float G = Smith_G(NdotV, NdotL, roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3(1.f, 1.f, 1.f) - F);

        switch (surfType) {
            case BSDF_DIFFUSE: {
                if (illuType == BSDF_PHONG || illuType == BSDF_BLINN_PHONG) {
                    return baseColor / PI;
                } else {
                    return NF * (1 - metallic) * baseColor / PI;
                }
            }
            case BSDF_GLOSSY: {
                if (illuType == BSDF_PHONG) {
                    // ideal reflected direction
                    Vec3<float> R = N * 2 * NdotL - L;
                    float VdotR = dot(V, R);

                    // !NOTE: normalize Phong/Blinn-Phong to form a valid BRDF, which ensures the integrated reflectance over the hemisphere remains ≤ 1.
                    // !      diffuse term is divided by π so that a pure white Lambert surface
                    // !      specular term uses the empirical normalization factor (shininess + 8) / (8π) or (shininess + 2) / (2π)
                    return baseColor / PI + specular * ::powf(VdotR, shininess) * (shininess + 2.f) / (2.f * PI);;
                } else if (illuType == BSDF_BLINN_PHONG) {
                    return baseColor / PI + specular * ::powf(NdotH, shininess) * (shininess + 8.f) / (8.f * PI);;
                } else {
                    float denom = std::max(4.0f * NdotV * NdotL, EPSILON);
                    return NF * (1 - metallic) * baseColor / PI + F * D * G / denom;
                }
            }
            case BSDF_SPECULAR: {
                // H should be same as N for perfect reflection
                // assert(distance(N, H) < EPSILON);
                // !NOTE: F already encodes the color and reflectance
                // !      so for perfect specular reflection, return F directly—no diffuse or albedo scaling needed.
                return F;
            }
            default: {
                return Vec3(0.f, 0.f, 0.f);
            }
        }
    }

    Vec3<float> Material::btdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV, float eta) const {
        // btdf surface type
        uint surfType = type & surfMask;

        // material color
        Vec3<float> baseColor = getBaseColor(UV);
        // Fresnel reflectance at normal incidence
        Vec3<float> F0 = getFresnel0(baseColor);

        // cosine constants
        float NdotL = dot(N, L);
        float NdotV = dot(N, V);
        float NdotH = dot(N, H);
        float LdotH = dot(L, H);
        float VdotH = dot(V, H);

        // make sure V, H, N are in the same hemisphere, while L in the opposite one
        assert(NdotL <= 0 && NdotV >= 0 && NdotH >= 0);

        // return if total internal reflection occurs
        if ((1 - VdotH * VdotH) * eta * eta > 1.f ) {
            return Vec3(0.f, 0.f, 0.f);
        }

        float D = GGX_D(NdotH, roughness);
        float G = Smith_G(NdotV, ::fabsf(NdotL), roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3(1.f, 1.f, 1.f) - F);

        switch (surfType) {
            case BSDF_SPECULAR: {
                // H should be same as N for perfect transmission
                // assert(distance(N, H) < EPSILON);
                return NF / (eta * eta);
            }
            case BSDF_GLOSSY: {
                float denom = std::max(::powf(eta * VdotH + LdotH, 2.f) * fabsf(NdotV * NdotL), EPSILON);
                Vec3<float> nom = NF * D * G * eta * eta * fabsf(VdotH * LdotH);
                return nom / denom;
            }
            default: {
                return Vec3(0.f, 0.f, 0.f);
            }
        }
    }

    float Material::GGX_D(float NdotH, float roughness) {
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float NdotH2 = NdotH * NdotH;
        
        float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
        denom = PI * denom * denom;
        return alpha2 / denom;
    }

    Vec3<float> Material::Fresnel_Schlick(float cosTheta, const Vec3<float>& F0) {
        return F0 + (Vec3<float>(1.f, 1.f, 1.f) - F0) * ::powf(1.0f - cosTheta, 5.0f);
    }

    float  Material::Smith_G(float NdotV, float NdotL, float roughness) {
        // SIGGRAPH 2013：UE4
        auto GeometrySchlickGGX = [roughness](float cosTheta) {
            float r = (roughness + 1.0f);
            float k = (r * r) / 8.0f;
            
            float nom = cosTheta;
            float denom = cosTheta * (1.0f - k) + k;
            
            return nom / denom;
        };

        float ggx1 = GeometrySchlickGGX(NdotV);
        float ggx2 = GeometrySchlickGGX(NdotL);
        
        return ggx1 * ggx2;
    }

} // namespace spt
