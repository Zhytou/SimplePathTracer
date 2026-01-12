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
        if (roughness == 0 && illuType != BSDF_MICROFACET) {
            roughness = ::sqrtf(10.f / (shininess + 10.f)); // roughness for phong model
        }
        
        // transparency
        transparency = 1 - mtl.dissolve;

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
        if (transparency < 0.3f) {
            type = type | BSDF_REFLECTION;
        } else {
            type = type | (BSDF_REFLECTION | BSDF_TRANSIMISSION);
        }
        // surface type
        if (roughness < 0.01f || (type & BSDF_TRANSIMISSION)) {
            // !NOTE: if surface is transparent, must be specular
            // TODO: add GLOSSY trasmission
            type = type | BSDF_SPECULAR;
        } else if (roughness > 0.9f) {
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

    /**
     * @brief Samples a direction for Monte Carlo integration of the material's BSDF.
     *
     * This function performs importance sampling based on the material's surface.
     * For example, it uses cosine-weighted hemisphere sampling for diffuse lobes.
     *
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * 
     * @return Vec3<float> Sampled outgoing direction (pointing AWAY from the surface).
     */
    Vec3<float> Material::scatter(const Vec3<float> &V, const Vec3<float> &N) const {
        // bsdf scatter type
        uint scatType = type & scatMask;

        auto L_t = transmit(V, N);
        auto L_r = reflect(V, N);
        float prob = rand(1.f);
        
        if ((scatType & BSDF_TRANSIMISSION) && prob < transparency) {
            return L_t;
        } else {
            return L_r;
        }
    }

    Vec3<float> Material::reflect(const Vec3<float> &V, const Vec3<float> &N) const {
        Vec3<float> L(0.f, 0.f, 0.f);

        // bsdf surface type
        uint surfType = type & surfMask;

        switch (surfType) {
            // perfect specular reflection
            case BSDF_SPECULAR: {
                // construct L
                float NdotV = dot(N, V);
                L = normalize(N * 2 * NdotV - V);
                // validate
                // assert(distance(normalize(V + L),  N) < EPSILON);
            }
            break;
            // glossy reflection
            case BSDF_GLOSSY: {
                // GGX importance sampling
                Vec3<float> H = sample(V, N, "GGX");

                float HdotV = dot(H, V);
                float HdotN = dot(H, N);

                // construct L
                L = normalize(H * 2 * HdotV - V);
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
            // total internal reflection
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

    float Material::pdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L) const {
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
                Vec3<float> H = normalize(V + L);
                float HdotV = dot(H, V);
                float HdotN = dot(H, N);
                float D = GGX_D(HdotN, roughness);
                float denom = std::max(4 * HdotV, EPSILON);
                PDF = D * HdotN / denom; 

                // TODO: add pdf calculation for glossy transimission
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

            float cosTheta = sqrtf((1 - a) / (1 + (alpha2 - 1) * a));
            float sinTheta = sqrtf(1 - cosTheta * cosTheta);
            float Phi = 2 * PI * b;

            localDir = {cosf(Phi) * sinTheta, sinf(Phi) * sinTheta, cosTheta};
        } else if (mode == "COSINE") {
            float cosTheta = sqrtf(a);
            float sinTheta = sqrtf(1 - cosTheta * cosTheta);
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
        Vec3<float> bsdf(0, 0, 0);

        if ((type & BSDF_TRANSIMISSION)) { // transimission
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
        } else{ // reflection 
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

        // mtl info
        Vec3<float> baseColor = getBaseColor(UV);
    
        // cosine constants
        float NdotL = dot(N, L);
        float NdotV = dot(N, V);
        float NdotH = dot(N, H);
        float LdotH = dot(L, H);
        float VdotH = dot(V, H);
        
        // all the directions must be in same hemisphere
        assert(NdotL >= 0 && NdotV >= 0 && NdotH >= 0);
        
        Vec3<float> F0(0.04f, 0.04f, 0.04f);
        F0 = F0 * (1 - metallic) + baseColor * metallic; // mix
    
        float D = GGX_D(NdotH, roughness);
        float G = Smith_G(NdotV, NdotL, roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3(1.f, 1.f, 1.f) - F);

        switch (surfType) {
            case BSDF_DIFFUSE: {
                if (illuType == BSDF_PHONG || illuType == BSDF_BLINN_PHONG) {
                    return baseColor;
                } else {
                    return NF * (1 - metallic) * (1 - transparency) * baseColor / PI;
                }
            }
            case BSDF_GLOSSY: {
                if (illuType == BSDF_PHONG) {
                    // ideal reflected direction
                    Vec3<float> R = N * 2 * NdotL - L;
                    float VdotR = dot(V, R);
                    return baseColor + specular * shininess * ::powf(VdotR, shininess);
                } else if (illuType == BSDF_BLINN_PHONG) {
                    return baseColor + specular * shininess * ::powf(NdotH, shininess);
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

        // mtl info
        Vec3<float> baseColor = getBaseColor(UV);
    
        // cosine constants
        float NdotL = dot(N, L);
        float NdotV = dot(N, V);
        float NdotH = dot(N, H);
        float LdotH = dot(L, H);
        float VdotH = dot(V, H);

        // make sure V, H, N are in the same hemisphere, while L in the opposite one
        assert(NdotL <= 0 && NdotV >= 0 && NdotH >= 0);

        if (1 - VdotH * VdotH == eta * eta * (1 - LdotH * LdotH)) {
            return Vec3(0.f, 0.f, 0.f);
        }

        Vec3<float> F0(eta, eta, eta);
        F0 = pow((F0 - Vec3(1.f, 1.f, 1.f)) / (F0 + Vec3(1.f, 1.f, 1.f)), 2.f);

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
                Vec3<float> nom = NF * D * G * eta * eta * transparency * fabsf(VdotH * LdotH);
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
