#include "Material.hpp"

#include <cassert>
#include <iostream>
#include <algorithm>
#include <tiny_obj_loader.h>

#include "Random.hpp"

namespace spt
{
    uint Material::scatMask = 0b00011;

    uint Material::surfMask = 0b11100;

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
     * @brief Samples a direction and its PDF for Monte Carlo integration of the material's BxDF.
     *
     * This function performs importance sampling based on the material's surface.
     * For example, it uses cosine-weighted hemisphere sampling for diffuse lobes.
     *
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * 
     * @return std::pair<Vec3<float>, float> 
     *         - First:  Sampled outgoing direction (L) pointing AWAY from the surface.
     *         - Second: Probability density (PDF) of the sampled direction.
     */
    std::pair<Vec3<float>, float> Material::sample(const Vec3<float> &V, const Vec3<float> &N) const {
        // bxdf scatter type
        uint scatType = type & scatMask;

        switch (scatType) {
            case (BSDF_REFLECTION | BSDF_TRANSIMISSION): {
                if (randFloat(1) < 0.5) {
                    return reflect(V, N);
                } else {
                    return transmit(V, N);
                }
            }
            case BSDF_REFLECTION: {
               return reflect(V, N);
            }
            case BSDF_TRANSIMISSION: {
                return transmit(V, N);
            }
            default: {
                return {Vec3<float>(0.f, 0.f, 0.f), 0.f};
            }
        }
    }

    /**
     * @brief Get the direction of reflected light.
     *
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * 
     * @return std::pair<Vec3<float>, float> 
     *         - First:  Sampled outgoing direction (L) pointing AWAY from the surface.
     *         - Second: Probability density (PDF) of the sampled direction.
     */
    std::pair<Vec3<float>, float> Material::reflect(const Vec3<float> &V, const Vec3<float> &N) const {
        Vec3<float> L(0.f, 0.f, 0.f);
        float PDF = 0.f;

        // bxdf surface type
        uint surfType = type & surfMask;

        switch (surfType) {
            // perfect specular reflection
            case BSDF_SPECULAR: {
                // construct L
                float NdotV = Vec3<float>::dot(N, V);
                L = Vec3<float>::normalize(N * 2 * NdotV - V);
                // validate
                // assert(Vec3<float>::distance(Vec3<float>::normalize(V + L),  N) < EPSILON);

                // only one possible direction
                PDF = 1;
            }
            break;
            // glossy reflection (CGX importance sampling)
            case BSDF_GLOSSY: {
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
                L = Vec3<float>::normalize(H * 2 * HdotV - V);

                // calculate L's pdf with the Jacobian of the reflection mapping
                float D = GGX_D(H, N, roughness);
                float denom = std::max(4 * HdotV * HdotV, EPSILON);
                PDF = D * HdotN / denom; 
            }
            break;
            // diffuse reflection (COSINE importance sampling)
            case BSDF_DIFFUSE: {
                // COSINE importance sampling
                Vec3<float> localL = sampleCosine();

                // orthogonal basis
                Vec3<float> E1 = Vec3<float>::cross(V, N).normalize();
                Vec3<float> E2 = Vec3<float>::cross(E1, N).normalize();
                // convert L to world coordinates
                L = E1 * localL.x + E2 * localL.y + N * localL.z;
                
                float NdotL = Vec3<float>::dot(N, L);

                if (NdotL < EPSILON) {
                    L = Vec3<float>(0.f, 0.f, 0.f);
                    PDF = 0.f;
                } else {
                    PDF = NdotL / PI;
                }
            }
            break;
        }

        return {L, PDF};
    }

    /**
     * @brief Get the direction of transimitted light.
     *
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * 
     * @return std::pair<Vec3<float>, float> 
     *         - First:  Sampled outgoing direction (L) pointing AWAY from the surface.
     *         - Second: Probability density (PDF) of the sampled direction.
     * 
     * @note Snell's law is sinThetaI / sinThetaT = iorT / iorI
     */
    std::pair<Vec3<float>, float> Material::transmit(const Vec3<float> &V, const Vec3<float> &N) const {
        Vec3<float> L(0.f, 0.f, 0.f);
        float PDF = 0.f;

        // bxdf surface type
        uint surfType = type & surfMask;

        switch (surfType) {
            // perfect specular trasimission
            case BSDF_SPECULAR: {
                // cosine incident theta
                float cosThetaI = Vec3<float>::dot(N, V); 

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
                    break;
                }

                // construct L
                float cosThetaT = sqrtf(1 - sin2ThetaT);
                L = - V * eta + NN * (eta * cosThetaI - cosThetaT);
                // only one possible direction
                PDF = 1;
            }
            break;
            
        }

        return {L, PDF};
    }

    /**
     * @brief Evaluates the BxDF value for given direction and point.
     * 
     * @param V     [in] Outgoing view direction (pointing AWAY from the surface). Must be normalized.
     * @param N     [in] Surface normal. Must be normalized.
     * @param L     [in] Incident light direction (pointing AWAY from the surface). Must be normalized.
     * @param UV    [in] Texture coordinates.
     * 
     * @return Vec3<float> The computed BxDF value.
     */
    Vec3<float> Material::eval(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec2<float>& UV) const {
        Vec3<float> bxdf(0, 0, 0);

        // reflection
        if ((type & BSDF_REFLECTION)) {
            // half vector
            Vec3<float> H = Vec3<float>::normalize(V + L);

            // make sure V, L, N, H in the same hemisphere
            if ((Vec3<float>::dot(V, N) > 0) && (Vec3<float>::dot(L, N) > 0)) {
                // brdf
                bxdf += brdf(V, N, L, H, UV);
            }
        } 

        // transimission
        if ((type & BSDF_TRANSIMISSION)) {
            // create a 'temporary' normal, same hemishpere with V
            Vec3<float> NN = (Vec3<float>::dot(N, V) > 0) ? N : -N;

            // ratio of ior
            float eta = (Vec3<float>::dot(N, V) > 0) ? 1.0f / ior : ior;

            // half vector
            Vec3<float> H = Vec3<float>::normalize(V * eta + L);

            // make sure H is in same hemisphere with NN
            H = (Vec3<float>::dot(NN, H) > 0) ? H : -H;

            // make sure V and N in the same hemisphere, while L in the opposite one
            if ((Vec3<float>::dot(V, NN) > 0) && (Vec3<float>::dot(L, NN) < 0)) {
                // btdf
                bxdf += btdf(V, NN, L, H, UV, eta);
            }
        }

        return bxdf;
    }

    /**
     * @brief Evaluates the BRDF (Bidirectional Reflectance Distribution Function) for a given material.
     *
     * @param V   [in] View direction (pointing AWAY from the surface). Must be normalized.
     * @param N   [in] Surface normal (pointing AWAY from the surface). Must be normalized.
     * @param L   [in] Light direction (pointing AWAY from the surface). Must be normalized.
     * @param H   [in] Half-vector (normalized midpoint between V and L). Must be normalized.
     * @param UV  [in] Texture coordinates.
     *
     * @return Vec3<float> The BRDF value (RGB color) for the given input directions.
     * 
     * @note All direction vectors (V, N, L, H) in the SAME hemisphere.
     */
    Vec3<float> Material::brdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV) const {
        // bxdf surface type
        uint surfType = type & surfMask;

        // mtl info
        Vec3<float> baseColor = getAlbedo(UV);
    
        // cosine constants
        float NdotL = Vec3<float>::dot(N, L);
        float NdotV = Vec3<float>::dot(N, V);
        float NdotH = Vec3<float>::dot(N, H);
        float LdotH = Vec3<float>::dot(L, H);
        float VdotH = Vec3<float>::dot(V, H);
        
        // all the directions must be in same hemisphere
        assert(NdotL >= 0 && NdotV >= 0 && NdotH >= 0);
        
        Vec3<float> F0 = Vec3<float>(0.04, 0.04, 0.04);
        F0 = F0 * (1 - metallic) + baseColor * metallic; // mix
    
        float D = GGX_D(H, N, roughness);
        float G = Smith_G(NdotV, NdotL, roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3<float>(1, 1, 1) - F);

        switch (surfType) {
            case BSDF_DIFFUSE: {
                return baseColor / PI;
            }
            case BSDF_GLOSSY: {
                float denom = std::max(4.0f * NdotV * NdotL, EPSILON);
                return NF * (1 - metallic) * baseColor / PI + F * D * G / denom;
            }
            case BSDF_SPECULAR: {
                // H should be same as N for perfect reflection
                // assert(Vec3<float>::distance(N, H) < EPSILON);

                return F * (baseColor / PI) / NdotV;
            }
            default: {
                return Vec3<float>(0.f, 0.f, 0.f);
            }
        }
    }

    /**
     * @brief Evaluates the BTDF (Bidirectional transmittance distribution function) for a given material.
     * 
     * @param V   [in] View direction (pointing AWAY from surface). Must be normalized.
     * @param N   [in] Geometric normal (pointing AWAY from surface). Must be normalized.
     * @param L   [in] Light direction (pointing AWAY from surface). Must be normalized.
     * @param H   [in] Half-vector (normalized midpoint: normalize(V + L)). Must be normalized.
     * @param UV  [in] Texture coordinates.
     * 
     * @return Vec3<float> BTDF value (RGB color).
     * 
     * @note V and N are in the SAME hemisphere (V·N ≥ 0), while L is in the OPPOSITE hemisphere (L·N ≤ 0).
     */
    Vec3<float> Material::btdf(const Vec3<float> &V, const Vec3<float> &N, const Vec3<float> &L, const Vec3<float>& H, const Vec2<float>& UV, float eta) const {
        // bxdf surface type
        uint surfType = type & surfMask;

        // mtl info
        Vec3<float> baseColor = getAlbedo(UV);
    
        // cosine constants
        float NdotL = Vec3<float>::dot(N, L);
        float NdotV = Vec3<float>::dot(N, V);
        float NdotH = Vec3<float>::dot(N, H);
        float LdotH = Vec3<float>::dot(L, H);
        float VdotH = Vec3<float>::dot(V, H);

        // make sure V, H, N are in the same hemisphere, while L in the opposite one
        assert(NdotL <= 0 && NdotV >= 0 && NdotH >= 0);

        Vec3<float> F0 = Vec3<float>(0.04, 0.04, 0.04);
        F0 = F0 * (1 - metallic) + baseColor * metallic; // mix
    
        float D = GGX_D(H, N, roughness);
        float G = Smith_G(NdotV, NdotL, roughness);
        Vec3<float> F = Fresnel_Schlick(VdotH, F0); 
        Vec3<float> NF = (Vec3<float>(1, 1, 1) - F);

        switch (surfType) {
            case BSDF_SPECULAR: {
                // H should be same as N for perfect transmission
                // assert(Vec3<float>::distance(N, H) < EPSILON);

                return NF * transparency / ((eta * eta) * VdotH);
            }
            case BSDF_GLOSSY: {
                return Vec3<float>(0.f, 0.f, 0.f);
            }
            default: {
                return Vec3<float>(0.f, 0.f, 0.f);
            }
        }
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
