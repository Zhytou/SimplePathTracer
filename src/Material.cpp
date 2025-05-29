#include "Material.hpp"

#include <cassert>
#include <algorithm>
#include <tiny_obj_loader.h>

#include "Random.hpp"

namespace spt
{
    Material::Material(const tinyobj::material_t& mtl, const std::string& dir) {
        // name
        name = mtl.name;

        // emissive
        emissive = false;
        
        // Kd -> base color
        albedo = mtl.diffuse;

        // Ks
        metallic = 0.5;
        
        // Ns -> roughness
        roughness = mtl.roughness;

        // ior
        ior = mtl.ior;

        // Kd Texture
        if (!mtl.diffuse_texname.empty()) {
            auto name = dir+mtl.diffuse_texname;
            tex = Texture::getInstance(name);
        } else {
            tex = nullptr;
        }
    }

    bool Material::isEmissive() const { 
        return emissive;
    }

    void Material::setEmission(Vec3<float> e) { 
        emissive = true;
        emission = e;
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

    std::pair<Vec3<float>, float> Material::sample(const Vec3<float> &wi, const Vec3<float> &n, BxDFType type) const {
        Vec3<float> wo(0.f, 0.f, 0.f);
        float pdf = 0.f;

        float cosi = Vec3<float>::dot(wi, n);
        if (cosi > 0) {
            return {wo, pdf};
        }

        switch (type) {
            // perfect specular reflection
            case BSDF_SPECULAR_R: {
                wo = wi - n * 2 * cosi;
                pdf = 1;
            }
                break;
            // perfect specular trasimission
            case BSDF_SPECULAR_T: {
                double etai = 1, etat = ior;
                Vec3<float> nn;

                if (cosi < 0) {
                    cosi = -cosi;
                    nn = n;
                } else {
                    std::swap(etai, etat);
                    nn = -n;
                }

                float eta = etai / etat;
                float cos2t = 1 - eta * eta * (1 - cosi * cosi);

                if (cos2t < 0) {
                    wo = Vec3<float>(0, 0, 0);
                    pdf = 0.f;
                } else {
                    float cost = sqrtf(cos2t);
                    wo = wi * eta + nn * (eta * cosi - cost);
                    pdf = 1.f;
                }
            }
                break;
            // glossy reflection
            case BSDF_GLOSSY_R: {

            }
                break;
            // diffuse reflection
            case BSDF_DIFFUSE_R: {
                Vec3<float> v1 = Vec3<float>::cross(wi, n).normalize();
                Vec3<float> v2 = Vec3<float>::cross(v1, n).normalize();

                float a = randFloat(1), b = randFloat(1);
                float theta =  2 * PI * a;

                float x = cos(theta) * sqrtf(b);
                float y = sin(theta) * sqrtf(b);
                float z = sqrt(1-b);

                wo = v1 * x + v2 * y + n * z;
                float coso = Vec3<float>::dot(n, wo);

                if (coso < EPSILON) {
                    wo = Vec3<float>(0.f, 0.f, 0.f);
                    pdf = 0.f;
                } else {
                    pdf = coso / PI;
                }
            }
                break;
        }

        return {wo, pdf};
    }

    Vec3<float> Material::eval(const Vec3<float> &wi, const Vec3<float> &n, const Vec3<float> &wo, const Vec2<float>& uv,BxDFType type) const {
        Vec3<float> bxdf(0, 0, 0);

        // reflection mask
        int mask = BSDF_DIFFUSE_R | BSDF_GLOSSY_R | BSDF_SPECULAR_R;

        // half vector
        Vec3<float> wh;
        if (type & mask) {
            wh = Vec3<float>::normalize(-wi+wo);
        } else {
            float eta = (Vec3<float>::dot(n, wi) > 0) ? 1.0f / ior : ior;
            wh = Vec3<float>::normalize(wi + wo * eta);
        }

        // cosine constants
        float NdotL = std::max(Vec3<float>::dot(n, -wi), 0.0f);
        float NdotV = std::max(Vec3<float>::dot(n, wo), 0.0f);
        float NdotH = std::max(Vec3<float>::dot(n, wh), 0.0f);
        float LdotH = std::max(Vec3<float>::dot(-wi, wh), 0.0f);
        float VdotH = std::max(Vec3<float>::dot(wo, wh), 0.0f);

        // mtl info
        Vec3<float> uvAlbedo = getAlbedo(uv);
        // metalic = getMetallic(uv)

        Vec3<float> F0 = Vec3<float>(0.04, 0.04, 0.04);
        F0 = F0 * (1 - metallic) + uvAlbedo * metallic; // mix
    
        float D = GGX_D(wh, n, roughness);
        float G = Smith_G(wi, wo, n, roughness);
        Vec3<float> F = Fresnel_Schlick(LdotH, F0); // specular
        Vec3<float> NF = (Vec3<float>(1, 1, 1) - F); // diffuse + trasimission

        switch (type) {
            case BSDF_DIFFUSE_R: {
                bxdf = albedo / PI * NdotL;
            }
            break;
            case BSDF_GLOSSY_R: {
                float denom = std::max(4.0f * NdotV * NdotH, EPSILON);
                bxdf = NF * (1 - metallic) * albedo / PI * NdotL + F * D * G / denom;
            }
            break;
            case BSDF_SPECULAR_R: {

            }
            break;
            case (BSDF_GLOSSY_T | BSDF_SPECULAR_T): {
                float eta = (NdotL > 0) ? 1.0f / ior : ior; 
                float denom = std::max(powf(eta * LdotH + VdotH, 2), EPSILON);
                bxdf =  NF * metallic * D * G * eta * eta * VdotH / denom;
            }
            break;
        }

        return bxdf;
    }

    float Material::GGX_D(const Vec3<float>& wh, const Vec3<float>& n, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = std::max(Vec3<float>::dot(n, wh), 0.0f);
        float NdotH2 = NdotH * NdotH;
        
        float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
        return a2 / (PI * denom * denom);
    }

    Vec3<float> Material::Fresnel_Schlick(float cosTheta, Vec3<float> F0) {
        return F0 + (Vec3<float>(1.f, 1.f, 1.f) - F0) * pow(1.0f - cosTheta, 5.0f);
    }

    float Material::GeometrySchlickGGX(float NdotV, float roughness) {
        float r = (roughness + 1.0f);
        float k = (r * r) / 8.0f;
        
        float nom = NdotV;
        float denom = NdotV * (1.0f - k) + k;
        
        return nom / denom;
    }

    float  Material::Smith_G(const Vec3<float>& wi, const Vec3<float>& wo, const Vec3<float>& n, float roughness) {
        float NdotV = std::max(Vec3<float>::dot(n, wo), 0.0f);
        float NdotL = std::max(Vec3<float>::dot(n, wi), 0.0f);
        float ggx1 = GeometrySchlickGGX(NdotV, roughness);
        float ggx2 = GeometrySchlickGGX(NdotL, roughness);
        
        return ggx1 * ggx2;
    }

} // namespace spt
