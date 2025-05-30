#include "Material.hpp"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <tiny_obj_loader.h>

#include "Random.hpp"

namespace spt
{
    uint Material::mask = 0b11;
    
    Material::Material(const tinyobj::material_t& mtl, const std::string& dir) {
        // name
        name = mtl.name;

        // emissive
        emissive = false;
        
        // base color
        albedo = mtl.diffuse;

        // metallic
        metallic = mtl.metallic;
        
        // roughness
        roughness = mtl.roughness;

        // transparency
        transparency = 1 - mtl.dissolve;

        // ior
        ior = mtl.ior;

        // Kd Texture
        if (!mtl.diffuse_texname.empty()) {
            auto name = dir+mtl.diffuse_texname;
            tex = Texture::getInstance(name);
        } else {
            tex = nullptr;
        }

        // type
        // scatter type
        if (transparency == 0) {
            type = BSDF_REFLECTION;
        } else if (transparency == 1.0) {
            type = BSDF_TRANSIMISSION;
        } else {
            // 0 < transparency < 1
            type = (BSDF_REFLECTION | BSDF_TRANSIMISSION);
        }
        // surface type
        if (roughness == 1) {
            type = type | BSDF_DIFFUSE;
        } else if (roughness == 0) {
            type = type | BSDF_SPECULAR;
        } else {
            type = type | BSDF_GLOSSY;
        }
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
            return Vec3<float>(0, 0, 0);
        }
    }

    Vec3<float> Material::getAlbedo(Vec2<float> uv) const {
        if (tex == nullptr) {
            return albedo;
        } else {
            return tex->getColorAt(uv);
        }
    }

    /**
     * @brief Samples a direction and its PDF for Monte Carlo integration of the material's BSDF.
     *
     * This function performs importance sampling based on the material's BRDF/BTDF properties.
     * For PBR materials, it typically uses GGX distribution for glossy/specular lobes.
     *
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * 
     * @return std::pair<Vec3<float>, float> 
     *         - First:  Sampled outgoing direction (L) pointing AWAY from the surface.
     *         - Second: Probability density (PDF) of the sampled direction.
     */
    std::pair<Vec3<float>, float> Material::sample(const Vec3<float> &V, const Vec3<float> &N) const {
        Vec3<float> L(0.f, 0.f, 0.f);
        float pdf = 0.f;

        float NdotV = Vec3<float>::dot(N, V);
        if (NdotV < 0) {
            return {L, pdf};
        }

        switch (type) {
            // perfect specular reflection
            case (BSDF_SPECULAR | BSDF_REFLECTION): {
                // construct L
                L = Vec3<float>::normalize(N * 2 * NdotV - V);
                // assert(Vec3<float>::distance(Vec3<float>::normalize(V + L),  N) < EPSILON); // validate

                // only one possible direction
                pdf = 1;
            }
            break;
            // perfect specular trasimission
            case (BSDF_SPECULAR | BSDF_TRANSIMISSION): {

            }
            break;
            // glossy reflection (CGX importance sampling)
            case (BSDF_GLOSSY | BSDF_REFLECTION): {
                // CGX importance sampling
                Vec3<float> localH = sampleGGX(roughness);

                // orthogonal basis
                Vec3<float> E1 = Vec3<float>::cross(V, N).normalize();
                Vec3<float> E2 = Vec3<float>::cross(E1, N).normalize();
                // convert H to world coordinates
                Vec3<float> H = E1 * localH.x + E2 * localH.y + N * localH.z;

                float HdotV = Vec3<float>::dot(H, V);
                float HdotN = Vec3<float>::dot(H, N);

                if (HdotV < 0 || HdotN < 0) {
                    break; // return default
                }

                // construct L
                Vec3<float> L = Vec3<float>::normalize(H * 2 * HdotV - V);
                // assert(Vec3<float>::distance(Vec3<float>::normalize(V + L),  H) < EPSILON); // validate

                // calculate L's pdf with the Jacobian of the reflection mapping
                float D = GGX_D(H, N, roughness);
                float denom = std::max(4 * HdotV * HdotV, EPSILON);
                pdf = D * HdotN / denom; 
            }
            break;
            case (BSDF_GLOSSY | BSDF_TRANSIMISSION): {

            }
            break;
            // diffuse reflection (COSINE importance sampling)
            case (BSDF_DIFFUSE | BSDF_REFLECTION): {
                // COSINE importance sampling
                Vec3<float> localL = sampleCosine();

                // orthogonal basis
                Vec3<float> E1 = Vec3<float>::cross(V, N).normalize();
                Vec3<float> E2 = Vec3<float>::cross(E1, N).normalize();
                // convert L to world coordinates
                Vec3<float> L = E1 * localL.x + E2 * localL.y + N * localL.z;
                
                float NdotL = Vec3<float>::dot(N, L);

                if (NdotL < EPSILON) {
                    L = Vec3<float>(0.f, 0.f, 0.f);
                    pdf = 0.f;
                } else {
                    pdf = NdotL / PI;
                }
            }
            break;
        }

        return {L, pdf};
    }

    /**
     * @brief Evaluates the BRDF/BTDF value for given light and view directions.
     * 
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * @param L     [in] Incident light direction (pointing AWAY from the surface). Must be normalized.
     * @param UV    [in] Texture coordinates (optional). Default = (0,0) if unused.
     * 
     * @return Vec3<float> The computed BxDF value.
     */
    Vec3<float> Material::eval(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec2<float>& UV) const {
        Vec3<float> bxdf(0, 0, 0);

        // reflection
        if (type & BSDF_REFLECTION) {
            // half vector
            Vec3<float> H = Vec3<float>::normalize(V + L);

            // brdf
            bxdf += brdf(V, N, L, H, UV);
        } 

        // transimission
        if (type & BSDF_TRANSIMISSION) {
            // half vector
            float eta = (Vec3<float>::dot(N, V) > 0) ? 1.0f / ior : ior;
            Vec3<float> H = Vec3<float>::normalize(V + L * eta);
        
            // btdf
            bxdf += btdf(V, N, L, H, UV);
        }

        return bxdf;
    }

    Vec3<float> Material::brdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const {
        Vec3<float> brdf(0.f, 0.f, 0.f);

        // bxdf surface type
        uint stype = type - (type & mask);

        // mtl info
        Vec3<float> baseColor = getAlbedo(UV);
    
        // cosine constants
        float NdotL = std::max(Vec3<float>::dot(N, L), 0.0f);
        float NdotV = std::max(Vec3<float>::dot(N, V), 0.0f);
        float NdotH = std::max(Vec3<float>::dot(N, H), 0.0f);
        float LdotH = std::max(Vec3<float>::dot(L, H), 0.0f);
        float VdotH = std::max(Vec3<float>::dot(V, H), 0.0f);

        Vec3<float> F0 = Vec3<float>(0.04, 0.04, 0.04);
        F0 = F0 * (1 - metallic) + baseColor * metallic; // mix
    
        float D = GGX_D(H, N, roughness);
        float G = Smith_G(NdotV, NdotL, roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3<float>(1, 1, 1) - F);

        switch (stype) {
            case BSDF_DIFFUSE: {
                brdf = baseColor / PI;
            }
            break;
            case BSDF_GLOSSY: {
                float denom = std::max(4.0f * NdotV * NdotL, EPSILON);
                brdf = NF * (1 - metallic) * baseColor / PI + F * D * G / denom;
            }
            break;
            case BSDF_SPECULAR: {
                brdf = F * (baseColor / PI) / NdotV;
            }
            break;
            default: {
                brdf = Vec3<float>(0.f, 0.f, 0.f);
            }
        }

        return brdf;
    }

    Vec3<float> Material::btdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const {
        Vec3<float> btdf(0.f, 0.f, 0.f);

        // bxdf surface type
        uint stype = type - (type & mask);

        // mtl info
        Vec3<float> baseColor = getAlbedo(UV);
    
        // cosine constants
        float NdotL = std::max(Vec3<float>::dot(N, L), 0.0f);
        float NdotV = std::max(Vec3<float>::dot(N, V), 0.0f);
        float NdotH = std::max(Vec3<float>::dot(N, H), 0.0f);
        float LdotH = std::max(Vec3<float>::dot(L, H), 0.0f);
        float VdotH = std::max(Vec3<float>::dot(V, H), 0.0f);

        Vec3<float> F0 = Vec3<float>(0.04, 0.04, 0.04);
        F0 = F0 * (1 - metallic) + baseColor * metallic; // mix
    
        float D = GGX_D(H, N, roughness);
        float G = Smith_G(NdotV, NdotL, roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3<float>(1, 1, 1) - F);

        switch (stype) {
            case BSDF_GLOSSY: {
            }
            break;
            case BSDF_SPECULAR: {
            }
            break;
            default: {
                btdf = Vec3<float>(0.f, 0.f, 0.f);
            }
        }

        return btdf;
    }

    float Material::GGX_D(const Vec3<float>& H, const Vec3<float>& N, float roughness) {
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float NdotH = std::max(Vec3<float>::dot(N, H), 0.0f);
        float NdotH2 = NdotH * NdotH;
        
        float denom = (NdotH2 * (alpha2 - 1.0f) + 1.0f);
        return alpha2 / (PI * denom * denom);
    }

    Vec3<float> Material::Fresnel_Schlick(float cosTheta, Vec3<float> F0) {
        return F0 + (Vec3<float>(1.f, 1.f, 1.f) - F0) * pow(1.0f - cosTheta, 5.0f);
    }

    float Material::GeometrySchlickGGX(float cosTheta, float roughness) {
        float r = (roughness + 1.0f);
        float k = (r * r) / 8.0f;
        
        float nom = cosTheta;
        float denom = cosTheta * (1.0f - k) + k;
        
        return nom / denom;
    }

    float  Material::Smith_G(float NdotV, float NdotL, float roughness) {
        float ggx1 = GeometrySchlickGGX(NdotV, roughness);
        float ggx2 = GeometrySchlickGGX(NdotL, roughness);
        
        return ggx1 * ggx2;
    }

} // namespace spt
