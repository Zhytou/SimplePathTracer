#ifndef SPT_MATERIAL_HPP
#define SPT_MATERIAL_HPP

#include <format>
#include <memory>
#include <string>
#include <tiny_obj_loader.h>
#include <unordered_map>

#include "Image.hpp"
#include "Utils.hpp"

namespace spt {

enum MaterialType {
    MATERIAL_NONE = 0,

    MATERIAL_PHYSICS_CONDUCTIVE     = 0b0001,
    MATERIAL_PHYSICS_SEMICONDUCTIVE = 0b0010,
    MATERIAL_PHYSICS_DIELECTRIC     = 0b0100, // could be transparent, indicate with opacity

    MATERIAL_SURFACE_DIFFUSE  = 0b0001 << 4,
    MATERIAL_SURFACE_GLOSSY   = 0b0010 << 4,
    MATERIAL_SURFACE_SPECULAR = 0b0100 << 4,
};

constexpr int PHYSICS_MASK = 0b1111;

constexpr int SURFACE_MASK = 0b11110000;

class Material {
   public:
    Material(const std::string& name) : name(name) {}

    bool isDelta() const { return m_type & MATERIAL_SURFACE_SPECULAR; }
    bool isEmissive() const { return min(m_emission) > 0.f; }
    bool isTransmissive() const { return (m_type & (MATERIAL_PHYSICS_SEMICONDUCTIVE | MATERIAL_PHYSICS_DIELECTRIC)) && m_opacity < 1.0f && m_ior > 1.f; }
    std::string getName() const { return name; }
    std::string getTypeStr() const { return std::format("[physics_{} surface_{}]", m_type & MATERIAL_PHYSICS_CONDUCTIVE ? "conductive" : (m_type & MATERIAL_PHYSICS_SEMICONDUCTIVE ? "semiconductive" : "dielectric"), m_type & MATERIAL_SURFACE_DIFFUSE ? "diffuse" : (m_type & MATERIAL_SURFACE_GLOSSY ? "glossy" : "specular")); }
    Vec3<float> getEmission() const { return m_emission; }
    Vec3<float> getAlbedo(const Vec2<float>& uv) const { return m_albedo; }
    float getMetallic(const Vec2<float>& uv) const { return m_metallic; }
    float getRoughness(const Vec2<float>& uv) const { return m_roughness; }
    float getOpacity() const { return m_opacity; }
    float getIOR() const { return m_ior; }
    void setType(MaterialType type) { m_type = type; }
    void setEmission(Vec3<float> e) { m_emission = e; }
    void setAlbedo(Vec3<float> a) { m_albedo = a; }
    void setIOR(float ior) { m_ior = ior; }
    void setOpacity(float o) { m_opacity = o; }
    void setRoughness(float r) { m_roughness = r; }
    void setMetallic(float m) { m_metallic = m; }

    Vec3<float> Fresnel_Zero(const Vec2<float>& UV) const;
    Vec3<float> Fresnel_Schlick(float NdotV, const Vec3<float>& F0) const;
    float GGX_D(float NdotH, const Vec2<float>& UV) const;
    float Smith_G(float NdotV, float NdotL, const Vec2<float>& UV) const;

    Vec3<float> bsdf(const Vec3<float>& wi, const Vec3<float>& n, const Vec3<float>& wo, const Vec2<float>& uv) const;
    Vec3<float> brdf(const Vec3<float>& wi, const Vec3<float>& n, const Vec3<float>& wo, const Vec3<float>& h, const Vec2<float>& uv) const;
    Vec3<float> brdf_tir(const Vec3<float>& wi, const Vec3<float>& n, const Vec3<float>& wo, const Vec3<float>& h, const Vec2<float>& uv) const;
    Vec3<float> btdf(const Vec3<float>& wi, const Vec3<float>& n, const Vec3<float>& wo, const Vec3<float>& h, const Vec2<float>& uv, float eta) const;

    Vec3<float> scatter(const Vec3<float>& wi, const Vec3<float>& n, const Vec2<float>& uv) const;
    Vec3<float> reflect(const Vec3<float>& wi, const Vec3<float>& n, const Vec2<float>& uv) const;
    Vec3<float> transmit(const Vec3<float>& wi, const Vec3<float>& n, const Vec2<float>& uv) const;

    Vec3<float> sample(const Vec3<float>& wi, const Vec3<float>& n, const Vec2<float>& uv, const std::string& mode) const;
    float pdf(const Vec3<float>& wi, const Vec3<float>& n, const Vec3<float>& wo, const Vec2<float>& uv) const;

   private:
    std::string name;
    MaterialType m_type = MATERIAL_NONE;
    Vec3<float> m_emission{0.f, 0.f, 0.f}; // emission color Ke
    Vec3<float> m_albedo{0.f, 0.f, 0.f};   // base color Kd
    float m_roughness = 0.f;               // roughness Pr
    float m_metallic  = 0.f;               // metalness Pm

    std::shared_ptr<Image<unsigned char>> m_emission_map  = nullptr;
    std::shared_ptr<Image<unsigned char>> m_albedo_map    = nullptr;
    std::shared_ptr<Image<unsigned char>> m_metallic_map  = nullptr;
    std::shared_ptr<Image<unsigned char>> m_roughness_map = nullptr;

    float m_ior     = 1.f; // index of refraction Ni
    float m_opacity = 1.f; // opacity D
};

} // namespace spt

#endif
